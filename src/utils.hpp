#pragma once

#include <boost/beast.hpp>
#include <boost/filesystem.hpp>
#include <memory>
#include <spdlog/logger.h>
#include <string>

namespace beast = boost::beast;
namespace http = beast::http;
namespace fs = boost::filesystem;

/**
 * Sets a generic HTTP response with custom status, body, and content type.
 * @param res The HTTP response object to modify
 * @param status The HTTP status code
 * @param body The response body content
 * @param content_type The content type of the response
 */
void set_response_generic(http::response<http::string_body> &res, http::status status, const std::string &body, const std::string &content_type);

/**
 * Sets 200 OK response with custom body and MIME type.
 * @param res The HTTP response object to modify
 * @param body The response body content
 * @param mime_type The MIME type of the response
 */
void set_response_200(http::response<http::string_body> &res, const std::string &body, const std::string &mime_type);

/**
 * Sets 403 Forbidden response.
 * @param res The HTTP response object to modify
 */
void set_response_403(http::response<http::string_body> &res);

/**
 * Sets 404 Not Found response.
 * @param res The HTTP response object to modify
 */
void set_response_404(http::response<http::string_body> &res);

/**
 * Sets 405 Method Not Allowed response.
 * @param res The HTTP response object to modify
 */
void set_response_405(http::response<http::string_body> &res);

/**
 * Sets 500 Internal Server Error response.
 * @param res The HTTP response object to modify
 */
void set_response_500(http::response<http::string_body> &res);

/**
 * Determines the MIME type based on file extension.
 * @param path The file path to analyze
 * @return The corresponding MIME type string
 */
std::string get_mime_type(const std::string &path);

/**
 * Reads the entire contents of a file into a string.
 * @param file_path Path to the file to read
 * @return File contents as string, empty on error
 */
std::string read_file(const std::string &file_path);

/**
 * Parses command line arguments to determine the directory to serve files from.
 * @param argc Number of command line arguments
 * @param argv Array of command line argument strings
 * @return Absolute path to the directory to serve files from
 */
fs::path parse_cmd(int argc, char *argv[]);

/**
 * Sets up the storage directory, creating it if it doesn't exist.
 * @param dir Path to the directory to set up
 */
void mkdir(const fs::path &dir);

/**
 * Initializes the global logger with stdout color sink.
 */
void init_logger();

extern std::shared_ptr<spdlog::logger> logger;
