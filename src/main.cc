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

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
namespace fs = boost::filesystem;
using tcp = boost::asio::ip::tcp;

std::atomic<bool> SHUTDOWN_REQUESTED{false};

std::string generate_directory_listing(const std::string &dir_path, const std::string &url_path) {
    std::string html = R"(
    <!DOCTYPE html>
    <html>
    <head>
        <title>Directory Listing</title>
        <style>
            body { font-family: Arial, sans-serif; margin: 20px; }
            .header { border-bottom: 1px solid #ccc; padding-bottom: 10px; }
            .file-list { margin-top: 20px; }
            .file-item { padding: 5px 0; }
            .file-item a { text-decoration: none; color: #0066cc; }
            .file-item a:hover { text-decoration: underline; }
            .directory { font-weight: bold; }
            .file { margin-left: 20px; }
        </style>
    </head>
    <body>
        <div class="header">
            <h1>Directory Listing for )" +
                       url_path + R"(</h1>
        </div>
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
                    html += R"(<div class="file-item directory"><a href=")" + link_path + R"(/">📁 )" + name + R"(/</a></div>)";
                } else {
                    html += R"(<div class="file-item file"><a href=")" + link_path + R"(">📄 )" + name + R"(</a></div>)";
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

void handle_request(http::request<http::string_body> &req, http::response<http::string_body> &res, const fs::path &files_dir) {
    logger->info("Handling {} request for: {}", req.method_string(), req.target());

    try {
        std::string target = std::string(req.target());
        fs::path file_path = files_dir / target.substr(1);

        if (!fs::exists(file_path)) {
            logger->warn("File not found: {}", file_path.string());
            res.result(http::status::not_found);
            res.set(http::field::content_type, "text/html");
            res.body() = "<html><body><h1>404 Not Found</h1><p>The requested resource was not found.</p></body></html>";
            res.prepare_payload();
            return;
        }

        if (fs::is_directory(file_path)) {
            fs::path index_path = file_path / "index.html";

            if (fs::exists(index_path) && fs::is_regular_file(index_path)) {
                std::string content = read_file(index_path.string());
                if (!content.empty()) {
                    res.result(http::status::ok);
                    res.set(http::field::content_type, "text/html");
                    res.body() = content;
                    res.prepare_payload();
                    return;
                }
            }

            std::string listing = generate_directory_listing(file_path.string(), target);
            res.result(http::status::ok);
            res.set(http::field::content_type, "text/html");
            res.body() = listing;
            res.prepare_payload();
            return;
        }

        if (fs::is_regular_file(file_path)) {
            std::string content = read_file(file_path.string());
            if (content.empty()) {
                logger->error("Failed to read file: {}", file_path.string());
                res.result(http::status::internal_server_error);
                res.set(http::field::content_type, "text/html");
                res.body() = "<html><body><h1>500 Internal Server Error</h1><p>Failed to read file.</p></body></html>";
                res.prepare_payload();
                return;
            }

            res.result(http::status::ok);
            res.set(http::field::content_type, get_mime_type(file_path.string()));
            res.body() = content;
            res.prepare_payload();
            return;
        }

        res.result(http::status::forbidden);
        res.set(http::field::content_type, "text/html");
        res.body() = "<html><body><h1>403 Forbidden</h1><p>Access denied.</p></body></html>";
        res.prepare_payload();

    } catch (const std::exception &e) {
        logger->error("Exception handling request: {}", e.what());
        res.result(http::status::internal_server_error);
        res.set(http::field::content_type, "text/html");
        res.body() = "<html><body><h1>500 Internal Server Error</h1><p>Server error occurred.</p></body></html>";
        res.prepare_payload();
    }
}

void session(tcp::socket socket, const fs::path &files_dir) {
    try {
        beast::flat_buffer buffer;
        http::request<http::string_body> req;
        http::read(socket, buffer, req);

        http::response<http::string_body> res;
        handle_request(req, res, files_dir);

        http::write(socket, res);
        socket.shutdown(tcp::socket::shutdown_send);
    } catch (const std::exception &e) {
        logger->error("Session error: {}", e.what());
    }
}

std::shared_ptr<tcp::acceptor> initialize_server(net::io_context &ioc, const fs::path &files_dir) {
    auto const address = net::ip::make_address("0.0.0.0");
    auto const port = static_cast<unsigned short>(8888);

    auto acceptor = std::make_shared<tcp::acceptor>(ioc, tcp::endpoint{address, port});

    logger->info("TinyFS HTTP Server starting on {}:{}", address.to_string(), port);
    logger->info("Serving files from: {}", files_dir.string());

    return acceptor;
}

void run_server(net::io_context &ioc, std::shared_ptr<tcp::acceptor> acceptor, const fs::path &files_dir) {
    std::function<void()> do_accept = [&]() {
        auto socket = std::make_shared<tcp::socket>(ioc);
        acceptor->async_accept(*socket, [socket, &do_accept, &files_dir](beast::error_code ec) {
            if (!ec && !SHUTDOWN_REQUESTED) {
                std::thread([socket, files_dir]() { session(std::move(*socket), files_dir); }).detach();
                do_accept();
            } else if (ec) {
                logger->error("Accept error: {}", ec.message());
            }
        });
    };

    do_accept();

    std::vector<std::thread> threads;
    threads.reserve(std::thread::hardware_concurrency());
    for (unsigned i = 0; i < std::thread::hardware_concurrency(); ++i) {
        threads.emplace_back([&ioc]() { ioc.run(); });
    }

    while (!SHUTDOWN_REQUESTED) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    logger->info("Shutting down server...");
    acceptor->close();
    ioc.stop();

    for (auto &t : threads) {
        if (t.joinable()) {
            t.join();
        }
    }

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
        net::io_context ioc{static_cast<int>(std::thread::hardware_concurrency())};
        auto acceptor = initialize_server(ioc, files_dir);
        run_server(ioc, acceptor, files_dir);

    } catch (const std::exception &e) {
        if (logger) {
            logger->error("Fatal error: {}", e.what());
        } else {
            std::cerr << "Fatal error: " << e.what() << std::endl;
        }
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
