#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/filesystem.hpp>
#include <boost/system.hpp>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
namespace fs = boost::filesystem;
using tcp = boost::asio::ip::tcp;

const std::string FILES_DIR = "/workspace/files";

std::string get_mime_type(const std::string &path) {
    if (path.size() >= 5 && path.substr(path.size() - 5) == ".html")
        return "text/html";
    if (path.size() >= 4 && path.substr(path.size() - 4) == ".htm")
        return "text/html";
    if (path.size() >= 4 && path.substr(path.size() - 4) == ".css")
        return "text/css";
    if (path.size() >= 3 && path.substr(path.size() - 3) == ".js")
        return "application/javascript";
    if (path.size() >= 5 && path.substr(path.size() - 5) == ".json")
        return "application/json";
    if (path.size() >= 4 && path.substr(path.size() - 4) == ".png")
        return "image/png";
    if (path.size() >= 4 && path.substr(path.size() - 4) == ".jpg")
        return "image/jpeg";
    if (path.size() >= 5 && path.substr(path.size() - 5) == ".jpeg")
        return "image/jpeg";
    if (path.size() >= 4 && path.substr(path.size() - 4) == ".gif")
        return "image/gif";
    if (path.size() >= 4 && path.substr(path.size() - 4) == ".txt")
        return "text/plain";
    return "application/octet-stream";
}

std::string read_file(const std::string &file_path) {
    std::ifstream file(file_path, std::ios::binary);
    if (!file) {
        return "";
    }
    return std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
}

void handle_request(http::request<http::string_body> &req, http::response<http::string_body> &res) {
    std::cout << "Handling request for: " << req.target() << std::endl;

    std::string target = std::string(req.target());
    if (target == "/") {
        target = "/index.html";
    }

    std::string file_path = FILES_DIR + target;
    if (!fs::exists(file_path) || !fs::is_regular_file(file_path)) {
        res.result(http::status::not_found);
        res.set(http::field::content_type, "text/plain");
        res.body() = "404 Not Found";
        res.prepare_payload();
        return;
    }

    std::string content = read_file(file_path);
    if (content.empty()) {
        res.result(http::status::internal_server_error);
        res.set(http::field::content_type, "text/plain");
        res.body() = "500 Internal Server Error";
        res.prepare_payload();
        return;
    }

    res.result(http::status::ok);
    res.set(http::field::content_type, get_mime_type(file_path));
    res.body() = content;
    res.prepare_payload();
}

void session(tcp::socket socket) {
    try {
        beast::flat_buffer buffer;
        http::request<http::string_body> req;
        http::read(socket, buffer, req);

        http::response<http::string_body> res;
        handle_request(req, res);

        http::write(socket, res);
        socket.shutdown(tcp::socket::shutdown_send);
    } catch (std::exception const &e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}

int main() {
    auto const address = net::ip::make_address("0.0.0.0");
    auto const port = static_cast<unsigned short>(8888); // hardcoded in docker-compose.yml

    net::io_context ioc{1};
    tcp::acceptor acceptor{ioc, {address, port}};

    std::cout << "TinyFS HTTP Server listening on " << address << ":" << port << std::endl;
    std::cout << "Serving files from " << FILES_DIR << std::endl;

    try {
        while (true) {
            tcp::socket socket{ioc};
            acceptor.accept(socket);
            std::thread([s = std::move(socket)]() mutable { session(std::move(s)); }).detach();
        }
    } catch (std::exception const &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
