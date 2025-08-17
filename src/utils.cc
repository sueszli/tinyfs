#include "utils.hpp"
#include <array>
#include <boost/program_options.hpp>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <mutex>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
#include <string_view>
#include <unordered_map>

namespace po = boost::program_options;

std::shared_ptr<spdlog::logger> logger;

ServerConfig ServerConfig::load_from_env() {
    ServerConfig config;
    if (const char *env_port = std::getenv("TINYFS_PORT")) {
        config.port = static_cast<unsigned short>(std::stoul(env_port));
    }
    if (const char *env_addr = std::getenv("TINYFS_ADDRESS")) {
        config.address = env_addr;
    }
    if (const char *env_poll = std::getenv("TINYFS_POLL_MS")) {
        config.shutdown_poll_ms = std::stoul(env_poll);
    }
    if (const char *env_max_size = std::getenv("TINYFS_MAX_FILE_MB")) {
        config.max_file_size_mb = std::stoul(env_max_size);
    }
    return config;
}

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

std::string read_file(const std::string &file_path, const ServerConfig &config) { return read_file_safe(file_path, config.max_file_size_mb); }

std::string read_file_safe(const std::string &file_path, size_t max_size_mb) {
    try {
        std::ifstream file(file_path, std::ios::binary | std::ios::ate);
        if (!file.is_open()) {
            if (logger) {
                logger->error("Failed to open file: {}", file_path);
            }
            return {};
        }

        const auto size = file.tellg();
        if (size < 0) {
            if (logger) {
                logger->error("Failed to get file size: {}", file_path);
            }
            return {};
        }

        const size_t max_size_bytes = max_size_mb * 1024 * 1024;
        if (static_cast<size_t>(size) > max_size_bytes) {
            if (logger) {
                logger->warn("File too large: {} ({} bytes, max {} MB)", file_path, static_cast<size_t>(size), max_size_mb);
            }
            return {};
        }

        std::string content;
        content.reserve(static_cast<size_t>(size));

        file.seekg(0, std::ios::beg);
        content.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());

        if (logger) {
            logger->debug("Successfully read file: {} ({} bytes)", file_path, static_cast<size_t>(size));
        }
        return content;
    } catch (const std::exception &e) {
        if (logger) {
            logger->error("Exception reading file {}: {}", file_path, e.what());
        }
        return {};
    }
}

fs::path parse_cmd(int argc, char *argv[]) {
    try {
        po::options_description desc("TinyFS HTTP Server Options");
        desc.add_options()("help,h", "Show this help message")("storage,s", po::value<std::string>()->required(), "Storage directory path (required)");

        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, desc), vm);

        if (vm.count("help")) {
            std::cout << desc << '\n';
            std::exit(EXIT_SUCCESS);
        }

        po::notify(vm);

        const auto storage_dir = vm["storage"].as<std::string>();
        if (storage_dir.empty()) {
            throw std::invalid_argument("Storage directory cannot be empty");
        }

        auto absolute_path = fs::absolute(storage_dir);
        if (logger) {
            logger->info("Using storage directory: {}", absolute_path.string());
        }
        return absolute_path;

    } catch (const po::required_option &e) {
        const std::string error_msg = "Missing required argument: " + std::string(e.what());
        if (logger) {
            logger->error(error_msg);
        } else {
            std::cerr << error_msg << '\n';
        }
        std::exit(EXIT_FAILURE);
    } catch (const po::error &e) {
        const std::string error_msg = "Command line error: " + std::string(e.what());
        if (logger) {
            logger->error(error_msg);
        } else {
            std::cerr << error_msg << '\n';
        }
        std::exit(EXIT_FAILURE);
    } catch (const std::exception &e) {
        const std::string error_msg = "Error parsing arguments: " + std::string(e.what());
        if (logger) {
            logger->error(error_msg);
        } else {
            std::cerr << error_msg << '\n';
        }
        std::exit(EXIT_FAILURE);
    }
}

void mkdir(const fs::path &dir) {
    try {
        if (!fs::exists(dir)) {
            fs::create_directories(dir);
            if (logger) {
                logger->info("Created directory: {}", dir.string());
            }
        } else if (!fs::is_directory(dir)) {
            throw std::runtime_error("Path exists but is not a directory");
        }
    } catch (const std::exception &e) {
        const std::string error_msg = "Failed to create directory " + dir.string() + ": " + e.what();
        if (logger) {
            logger->error(error_msg);
        } else {
            std::cerr << error_msg << '\n';
        }
        throw;
    }
}

void init_logger() {
    static std::once_flag flag;
    std::call_once(flag, []() {
        logger = spdlog::stdout_color_mt("tinyfs");
        logger->set_level(spdlog::level::info);
        logger->set_pattern("[%H:%M:%S] [%^%l%$] %v");
    });
}
