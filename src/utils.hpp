#pragma once

#include <boost/filesystem.hpp>
#include <memory>
#include <spdlog/logger.h>
#include <string>

namespace fs = boost::filesystem;

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
