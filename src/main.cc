#include "utils.hpp"
#include <atomic>
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/filesystem.hpp>
#include <boost/system.hpp>
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;

std::atomic<bool> SHUTDOWN_REQUESTED{false};

std::string generate_directory_listing(const std::string &dir_path, const std::string &url_path) {
    std::string html = R"(
    <!DOCTYPE html>
    <html><head><title>Directory Listing</title></head>
    <body>
        <div class="header"><h1>Directory Listing for )" +
                       url_path + R"(</h1> </div>
        <div class="file-list">
    )";

    if (url_path != "/") {
        std::string parent = url_path;
        if (parent.back() == '/')
            parent.pop_back();
        size_t pos = parent.find_last_of('/');
        if (pos != std::string::npos) {
            parent = parent.substr(0, pos + 1);
        } else {
            parent = "/";
        }
        html += R"(<div class="file-item directory"><a href=")" + parent + R"(">.. (Parent Directory)</a></div>)";
    }

    try {
        if (fs::exists(dir_path) && fs::is_directory(dir_path)) {
            std::vector<fs::directory_entry> entries;
            for (const auto &entry : fs::directory_iterator(dir_path)) {
                entries.push_back(entry);
            }

            std::sort(entries.begin(), entries.end(), [](const fs::directory_entry &a, const fs::directory_entry &b) {
                if (fs::is_directory(a) != fs::is_directory(b)) {
                    return fs::is_directory(a);
                }
                return a.path().filename() < b.path().filename();
            });

            for (const auto &entry : entries) {
                std::string name = entry.path().filename().string();
                std::string link_path = url_path;
                if (link_path.back() != '/')
                    link_path += "/";
                link_path += name;

                if (fs::is_directory(entry)) {
                    html += R"(<div class="file-item directory"><a href=")" + link_path + R"("> )" + name + R"( (Directory) /</a></div>)";
                } else {
                    html += R"(<div class="file-item file"><a href=")" + link_path + R"("> )" + name + R"( (File) </a></div>)";
                }
            }
        }
    } catch (const std::exception &e) {
        logger->error("Error listing directory {}: {}", dir_path, e.what());
        html += R"(<div class="file-item">Error reading directory</div>)";
    }

    html += R"(</div></body></html>)";
    return html;
}

void handle_request(http::request<http::string_body> &req, http::response<http::string_body> &res, const fs::path &files_dir, const ServerConfig &config) {
    logger->info("Received {} request for {}", req.method_string(), req.target());
    if (req.method() != http::verb::get) {
        logger->warn("Method not allowed: {}", req.method_string());
        set_response_405(res);
        return;
    }

    try {
        std::string target = std::string(req.target());
        fs::path file_path = files_dir / target.substr(1);
        if (!fs::exists(file_path)) {
            logger->warn("File not found: {}", file_path.string());
            set_response_404(res);
            return;
        }

        // directory mode
        if (fs::is_directory(file_path)) {
            fs::path index_path = file_path / "index.html";
            if (fs::exists(index_path) && fs::is_regular_file(index_path)) {
                std::string content = read_file(index_path.string(), config);
                if (!content.empty()) {
                    set_response_200(res, content, "text/html");
                    return;
                }
            }
            std::string listing = generate_directory_listing(file_path.string(), target);
            set_response_200(res, listing, "text/html");
            return;
        }

        // file mode
        if (fs::is_regular_file(file_path)) {
            std::string content = read_file_safe(file_path.string(), config.max_file_size_mb);
            if (content.empty()) {
                logger->error("Failed to read file: {}", file_path.string());
                set_response_500(res);
                return;
            }
            set_response_200(res, content, get_mime_type(file_path.string()));
            return;
        }

        // unreachable
        set_response_403(res);

    } catch (const std::exception &e) {
        logger->error("Exception handling request: {}", e.what());
        set_response_500(res);
    }
}

void session(tcp::socket socket, const fs::path &files_dir, const ServerConfig &config) {
    try {
        beast::flat_buffer buffer;
        http::request<http::string_body> req;
        http::read(socket, buffer, req);

        http::response<http::string_body> res;
        handle_request(req, res, files_dir, config);

        http::write(socket, res);
        socket.shutdown(tcp::socket::shutdown_send);
    } catch (const std::exception &e) {
        logger->error("Session error: {}", e.what());
    }
}

class ThreadPool {
  public:
    explicit ThreadPool(net::io_context &ioc) : ioc_(ioc) {
        threads_.reserve(std::thread::hardware_concurrency());
        for (unsigned i = 0; i < std::thread::hardware_concurrency(); ++i) {
            threads_.emplace_back([&ioc]() { ioc.run(); });
        }
    }

    ~ThreadPool() {
        for (auto &t : threads_) {
            if (t.joinable()) {
                t.join();
            }
        }
    }

    ThreadPool(const ThreadPool &) = delete;
    ThreadPool &operator=(const ThreadPool &) = delete;
    ThreadPool(ThreadPool &&) = delete;
    ThreadPool &operator=(ThreadPool &&) = delete;

  private:
    net::io_context &ioc_;
    std::vector<std::thread> threads_;
};

void run_server(net::io_context &ioc, std::shared_ptr<tcp::acceptor> acceptor, const fs::path &files_dir, const ServerConfig &config) {
    std::function<void()> do_accept = [&]() {
        auto socket = std::make_shared<tcp::socket>(ioc);
        acceptor->async_accept(*socket, [socket, &do_accept, &files_dir, &config](beast::error_code ec) {
            if (!ec && !SHUTDOWN_REQUESTED) {
                std::thread([socket, files_dir, config]() { session(std::move(*socket), files_dir, config); }).detach();
                do_accept();
            } else if (ec) {
                logger->error("Accept error: {}", ec.message());
            }
        });
    };
    do_accept();

    ThreadPool thread_pool(ioc);

    while (!SHUTDOWN_REQUESTED) {
        std::this_thread::sleep_for(std::chrono::milliseconds(config.shutdown_poll_ms));
    }

    logger->info("Shutting down server...");
    acceptor->close();
    ioc.stop();
    logger->info("Server shutdown complete");
}

void signal_handler(int signal) {
    logger->info("Received signal {}, initiating graceful shutdown...", signal);
    SHUTDOWN_REQUESTED = true;
}

void setup_signal_handlers() {
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);
}

int main(int argc, char *argv[]) {
    init_logger();
    fs::path files_dir = parse_cmd(argc, argv);
    mkdir(files_dir);
    setup_signal_handlers();

    try {
        ServerConfig config = ServerConfig::load_from_env();
        net::io_context ioc{static_cast<int>(std::thread::hardware_concurrency())};
        auto const address = net::ip::make_address(config.address);
        auto acceptor = std::make_shared<tcp::acceptor>(ioc, tcp::endpoint{address, config.port});
        logger->info("Serving files from {} on {}:{}", files_dir.string(), address.to_string(), config.port);
        run_server(ioc, acceptor, files_dir, config);

    } catch (const std::exception &e) {
        if (logger) {
            logger->error("Fatal error: {}", e.what());
        }
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
