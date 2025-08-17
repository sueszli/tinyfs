#include "utils.hpp"
#include <array>
#include <boost/program_options.hpp>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
#include <string_view>
#include <unordered_map>

namespace po = boost::program_options;

std::shared_ptr<spdlog::logger> logger;

void set_response_generic(http::response<http::string_body> &res, http::status status, const std::string &body, const std::string &content_type) {
    res.set(http::field::server, "TinyFS");
    res.result(status);
    res.set(http::field::content_type, content_type);
    res.body() = body;
    res.prepare_payload();
}
void set_response_200(http::response<http::string_body> &res, const std::string &body, const std::string &mime_type) { set_response_generic(res, http::status::ok, body, mime_type); }
void set_response_404(http::response<http::string_body> &res) { set_response_generic(res, http::status::not_found, "<html><body><h1>404 Not Found</h1><p>The requested resource was not found.</p></body></html>", "text/html"); }
void set_response_403(http::response<http::string_body> &res) { set_response_generic(res, http::status::forbidden, "<html><body><h1>403 Forbidden</h1><p>Access denied.</p></body></html>", "text/html"); }
void set_response_405(http::response<http::string_body> &res) { set_response_generic(res, http::status::method_not_allowed, "<html><body><h1>405 Method Not Allowed</h1><p>This method is not allowed.</p></body></html>", "text/html"); }
void set_response_500(http::response<http::string_body> &res) { set_response_generic(res, http::status::internal_server_error, "<html><body><h1>500 Internal Server Error</h1><p>Server error occurred.</p></body></html>", "text/html"); }

std::string get_mime_type(const std::string &path) {
    static const std::unordered_map<std::string_view, std::string_view> MIME_TYPES = {{".html", "text/html"}, {".htm", "text/html"}, {".css", "text/css"}, {".js", "application/javascript"}, {".json", "application/json"}, {".png", "image/png"}, {".jpg", "image/jpeg"}, {".jpeg", "image/jpeg"}, {".gif", "image/gif"}, {".txt", "text/plain"}};

    constexpr std::string_view DEFAULT_MIME_TYPE = "application/octet-stream";

    if (path.empty()) {
        return std::string(DEFAULT_MIME_TYPE);
    }

    const auto dot_pos = path.find_last_of('.');
    if (dot_pos == std::string::npos) {
        return std::string(DEFAULT_MIME_TYPE);
    }

    const std::string_view extension(path.data() + dot_pos, path.size() - dot_pos);
    const auto it = MIME_TYPES.find(extension);
    return it != MIME_TYPES.end() ? std::string(it->second) : std::string(DEFAULT_MIME_TYPE);
}

std::string read_file(const std::string &file_path) {
    try {
        std::ifstream file(file_path, std::ios::binary | std::ios::ate);
        if (!file.is_open()) {
            return {};
        }

        const auto size = file.tellg(); // alloc based on file size
        if (size < 0) {
            return {};
        }

        std::string content;
        content.reserve(static_cast<size_t>(size));

        file.seekg(0, std::ios::beg); // seek back to 0
        content.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());

        return content;
    } catch (const std::exception &e) {
        return {};
    }
}

fs::path parse_cmd(int argc, char *argv[]) {
    try {
        po::options_description desc("TinyFS HTTP Server Options");
        desc.add_options()("help,h", "Show this help message")("storage,s", po::value<std::string>()->default_value("workspace/files"), "Storage directory path (default: workspace/files)");

        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);

        if (vm.count("help")) {
            std::cout << desc << '\n';
            std::exit(EXIT_SUCCESS);
        }

        const auto storage_dir = vm["storage"].as<std::string>();
        return fs::absolute(storage_dir);

    } catch (const po::error &e) {
        std::cerr << "Command line error: " << e.what() << '\n';
        std::exit(EXIT_FAILURE);
    } catch (const std::exception &e) {
        std::cerr << "Error parsing arguments: " << e.what() << '\n';
        std::exit(EXIT_FAILURE);
    }
}

void mkdir(const fs::path &dir) {
    if (!fs::exists(dir)) {
        fs::create_directories(dir);
    }
}

void init_logger() {
    logger = spdlog::stdout_color_mt("tinyfs");
    logger->set_level(spdlog::level::info);
    logger->set_pattern("[%H:%M:%S] [%^%l%$] %v");
}
