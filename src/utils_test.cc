#include "utils.hpp"
#include <fstream>
#include <gtest/gtest.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
#include <vector>

// logger is defined in utils.cc

class ResponseFunctionTest : public ::testing::Test {
  protected:
    void SetUp() override { response = http::response<http::string_body>{}; }

    void TearDown() override {}

    http::response<http::string_body> response;
};

TEST_F(ResponseFunctionTest, SetResponse200) {
    const std::string body = "Test content";
    const std::string mime_type = "text/plain";

    set_response_200(response, body, mime_type);

    EXPECT_EQ(response.result(), http::status::ok);
    EXPECT_EQ(response.body(), body);
    EXPECT_EQ(response[http::field::content_type], mime_type);
    EXPECT_EQ(response[http::field::server], "TinyFS");
}

TEST_F(ResponseFunctionTest, SetResponse404) {
    set_response_404(response);

    EXPECT_EQ(response.result(), http::status::not_found);
    EXPECT_EQ(response[http::field::content_type], "text/html");
    EXPECT_EQ(response[http::field::server], "TinyFS");
    EXPECT_TRUE(response.body().find("404 Not Found") != std::string::npos);
}

TEST_F(ResponseFunctionTest, SetResponse403) {
    set_response_403(response);

    EXPECT_EQ(response.result(), http::status::forbidden);
    EXPECT_EQ(response[http::field::content_type], "text/html");
    EXPECT_EQ(response[http::field::server], "TinyFS");
    EXPECT_TRUE(response.body().find("403 Forbidden") != std::string::npos);
}

TEST_F(ResponseFunctionTest, SetResponse405) {
    set_response_405(response);

    EXPECT_EQ(response.result(), http::status::method_not_allowed);
    EXPECT_EQ(response[http::field::content_type], "text/html");
    EXPECT_EQ(response[http::field::server], "TinyFS");
    EXPECT_TRUE(response.body().find("405 Method Not Allowed") != std::string::npos);
}

TEST_F(ResponseFunctionTest, SetResponse500) {
    set_response_500(response);

    EXPECT_EQ(response.result(), http::status::internal_server_error);
    EXPECT_EQ(response[http::field::content_type], "text/html");
    EXPECT_EQ(response[http::field::server], "TinyFS");
    EXPECT_TRUE(response.body().find("500 Internal Server Error") != std::string::npos);
}

TEST_F(ResponseFunctionTest, SetResponseGeneric) {
    const std::string body = "Custom response";
    const std::string content_type = "application/json";

    set_response_generic(response, http::status::created, body, content_type);

    EXPECT_EQ(response.result(), http::status::created);
    EXPECT_EQ(response.body(), body);
    EXPECT_EQ(response[http::field::content_type], content_type);
    EXPECT_EQ(response[http::field::server], "TinyFS");
}

TEST_F(ResponseFunctionTest, SetResponseDefaultContentType) {
    const std::string body = "Default content type test";

    set_response_generic(response, http::status::accepted, body, "text/html");

    EXPECT_EQ(response.result(), http::status::accepted);
    EXPECT_EQ(response.body(), body);
    EXPECT_EQ(response[http::field::content_type], "text/html");
    EXPECT_EQ(response[http::field::server], "TinyFS");
}

class GetMimeTypeTest : public ::testing::Test {
  protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(GetMimeTypeTest, KnownExtensions) {
    EXPECT_EQ(get_mime_type("index.html"), "text/html");
    EXPECT_EQ(get_mime_type("page.htm"), "text/html");
    EXPECT_EQ(get_mime_type("style.css"), "text/css");
    EXPECT_EQ(get_mime_type("script.js"), "application/javascript");
    EXPECT_EQ(get_mime_type("data.json"), "application/json");
    EXPECT_EQ(get_mime_type("image.png"), "image/png");
    EXPECT_EQ(get_mime_type("photo.jpg"), "image/jpeg");
    EXPECT_EQ(get_mime_type("picture.jpeg"), "image/jpeg");
    EXPECT_EQ(get_mime_type("animation.gif"), "image/gif");
    EXPECT_EQ(get_mime_type("readme.txt"), "text/plain");
}

TEST_F(GetMimeTypeTest, PathsWithDirectories) {
    EXPECT_EQ(get_mime_type("/path/to/file.html"), "text/html");
    EXPECT_EQ(get_mime_type("../relative/path/style.css"), "text/css");
    EXPECT_EQ(get_mime_type("./current/dir/script.js"), "application/javascript");
    EXPECT_EQ(get_mime_type("deep/nested/path/image.png"), "image/png");
}

TEST_F(GetMimeTypeTest, MultipleDots) {
    EXPECT_EQ(get_mime_type("file.min.js"), "application/javascript");
    EXPECT_EQ(get_mime_type("style.bundle.css"), "text/css");
    EXPECT_EQ(get_mime_type("config.dev.json"), "application/json");
    EXPECT_EQ(get_mime_type("archive.tar.gz"), "application/octet-stream");
}

TEST_F(GetMimeTypeTest, EdgeCases) {
    // Empty string
    EXPECT_EQ(get_mime_type(""), "application/octet-stream");

    // No extension
    EXPECT_EQ(get_mime_type("filename"), "application/octet-stream");
    EXPECT_EQ(get_mime_type("README"), "application/octet-stream");

    // Only extension
    EXPECT_EQ(get_mime_type(".html"), "text/html");
    EXPECT_EQ(get_mime_type(".css"), "text/css");

    // Dot at end
    EXPECT_EQ(get_mime_type("filename."), "application/octet-stream");

    // Unknown extension
    EXPECT_EQ(get_mime_type("file.unknown"), "application/octet-stream");
    EXPECT_EQ(get_mime_type("file.xyz"), "application/octet-stream");
}

TEST_F(GetMimeTypeTest, HiddenFiles) {
    EXPECT_EQ(get_mime_type(".gitignore"), "application/octet-stream");
    EXPECT_EQ(get_mime_type(".env"), "application/octet-stream");
    EXPECT_EQ(get_mime_type(".hidden.html"), "text/html");
    EXPECT_EQ(get_mime_type(".config.json"), "application/json");
}

TEST_F(GetMimeTypeTest, LongPaths) {
    std::string long_path = "/very/long/path/with/many/nested/directories/that/goes/on/and/on/file.html";
    EXPECT_EQ(get_mime_type(long_path), "text/html");

    // Very long filename
    std::string long_filename(1000, 'a');
    long_filename += ".css";
    EXPECT_EQ(get_mime_type(long_filename), "text/css");
}

TEST_F(GetMimeTypeTest, ExtensionBoundaries) {
    // Shortest extension (.js)
    EXPECT_EQ(get_mime_type("a.js"), "application/javascript");

    // Longest extension (.jpeg)
    EXPECT_EQ(get_mime_type("a.jpeg"), "image/jpeg");

    // Similar extensions
    EXPECT_EQ(get_mime_type("file.htm"), "text/html");
    EXPECT_EQ(get_mime_type("file.html"), "text/html");
    EXPECT_EQ(get_mime_type("file.jpg"), "image/jpeg");
    EXPECT_EQ(get_mime_type("file.jpeg"), "image/jpeg");
}

class ReadFileTest : public ::testing::Test {
  protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(ReadFileTest, SuccessfulFileReading) {
    const std::string test_content = "Hello, World!\nThis is a test file.";
    const std::string temp_file = "/tmp/read_file_test.txt";

    std::ofstream out(temp_file);
    out << test_content;
    out.close();

    std::string result = read_file(temp_file);
    EXPECT_EQ(result, test_content);

    std::remove(temp_file.c_str());
}

TEST_F(ReadFileTest, NonExistentFile) {
    const std::string non_existent_file = "/tmp/this_file_does_not_exist_12345.txt";

    std::string result = read_file(non_existent_file);
    EXPECT_TRUE(result.empty());
}

TEST_F(ReadFileTest, EmptyFile) {
    const std::string temp_file = "/tmp/empty_file_test.txt";

    std::ofstream out(temp_file);
    out.close();

    std::string result = read_file(temp_file);
    EXPECT_TRUE(result.empty());

    std::remove(temp_file.c_str());
}

TEST_F(ReadFileTest, BinaryFileReading) {
    const std::string temp_file = "/tmp/binary_file_test.bin";
    const std::vector<char> binary_data = {0x00, 0x01, 0x02, static_cast<char>(0xFF), static_cast<char>(0xFE), static_cast<char>(0xFD)};

    std::ofstream out(temp_file, std::ios::binary);
    out.write(binary_data.data(), binary_data.size());
    out.close();

    std::string result = read_file(temp_file);
    EXPECT_EQ(result.size(), binary_data.size());

    for (size_t i = 0; i < binary_data.size(); ++i) {
        EXPECT_EQ(static_cast<unsigned char>(result[i]), static_cast<unsigned char>(binary_data[i]));
    }

    std::remove(temp_file.c_str());
}

TEST_F(ReadFileTest, FileWithSpecialCharacters) {
    const std::string temp_file = "/tmp/special_chars_test.txt";
    const std::string test_content = "Hello 世界!\nSpecial chars: áéíóú, ñüöä\nUnicode: ✓ ★ ♥ ☀";

    std::ofstream out(temp_file);
    out << test_content;
    out.close();

    std::string result = read_file(temp_file);
    EXPECT_EQ(result, test_content);

    std::remove(temp_file.c_str());
}

class ParseCmdTest : public ::testing::Test {
  protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(ParseCmdTest, DefaultStorageDirectory) {
    const char *argv[] = {"tinyfs"};
    int argc = 1;

    fs::path result = parse_cmd(argc, const_cast<char **>(argv));
    fs::path expected = fs::absolute("workspace/files");

    EXPECT_EQ(result, expected);
}

TEST_F(ParseCmdTest, CustomStorageDirectory) {
    const char *argv[] = {"tinyfs", "--storage", "/custom/path"};
    int argc = 3;

    fs::path result = parse_cmd(argc, const_cast<char **>(argv));
    fs::path expected = fs::absolute("/custom/path");

    EXPECT_EQ(result, expected);
}

TEST_F(ParseCmdTest, ShortOptionCustomStorageDirectory) {
    const char *argv[] = {"tinyfs", "-s", "/another/path"};
    int argc = 3;

    fs::path result = parse_cmd(argc, const_cast<char **>(argv));
    fs::path expected = fs::absolute("/another/path");

    EXPECT_EQ(result, expected);
}

TEST_F(ParseCmdTest, RelativePathConvertedToAbsolute) {
    const char *argv[] = {"tinyfs", "--storage", "relative/path"};
    int argc = 3;

    fs::path result = parse_cmd(argc, const_cast<char **>(argv));
    fs::path expected = fs::absolute("relative/path");

    EXPECT_EQ(result, expected);
}

TEST_F(ParseCmdTest, EmptyStorageDirectory) {
    const char *argv[] = {"tinyfs", "--storage", ""};
    int argc = 3;

    fs::path result = parse_cmd(argc, const_cast<char **>(argv));
    fs::path expected = fs::absolute("");

    EXPECT_EQ(result, expected);
}

class SetupStorageDirectoryTest : public ::testing::Test {
  protected:
    void SetUp() override {
        if (!logger) {
            logger = spdlog::stdout_color_mt("test_logger");
            logger->set_level(spdlog::level::info);
        }

        test_base_dir = fs::temp_directory_path() / "tinyfs_test_storage";
        if (fs::exists(test_base_dir)) {
            fs::remove_all(test_base_dir);
        }
    }

    void TearDown() override {
        if (fs::exists(test_base_dir)) {
            fs::remove_all(test_base_dir);
        }
    }

    fs::path test_base_dir;
};

TEST_F(SetupStorageDirectoryTest, CreatesNonExistentDirectory) {
    fs::path test_dir = test_base_dir / "new_directory";

    ASSERT_FALSE(fs::exists(test_dir));
    mkdir(test_dir);

    EXPECT_TRUE(fs::exists(test_dir));
    EXPECT_TRUE(fs::is_directory(test_dir));
}

TEST_F(SetupStorageDirectoryTest, ExistingDirectoryNoOp) {
    fs::path test_dir = test_base_dir / "existing_directory";

    fs::create_directories(test_dir);
    ASSERT_TRUE(fs::exists(test_dir));

    mkdir(test_dir);

    EXPECT_TRUE(fs::exists(test_dir));
    EXPECT_TRUE(fs::is_directory(test_dir));
}

TEST_F(SetupStorageDirectoryTest, CreatesNestedDirectories) {
    fs::path test_dir = test_base_dir / "level1" / "level2" / "level3";

    ASSERT_FALSE(fs::exists(test_dir));
    ASSERT_FALSE(fs::exists(test_base_dir / "level1"));

    mkdir(test_dir);

    EXPECT_TRUE(fs::exists(test_dir));
    EXPECT_TRUE(fs::is_directory(test_dir));
    EXPECT_TRUE(fs::exists(test_base_dir / "level1"));
    EXPECT_TRUE(fs::exists(test_base_dir / "level1" / "level2"));
}

TEST_F(SetupStorageDirectoryTest, HandlesRelativePath) {
    fs::path relative_path = "test_relative_dir";

    mkdir(relative_path);

    EXPECT_TRUE(fs::exists(relative_path));
    EXPECT_TRUE(fs::is_directory(relative_path));

    if (fs::exists(relative_path)) {
        fs::remove_all(relative_path);
    }
}

TEST_F(SetupStorageDirectoryTest, HandlesPathWithSpaces) {
    fs::path test_dir = test_base_dir / "directory with spaces";

    ASSERT_FALSE(fs::exists(test_dir));

    mkdir(test_dir);

    EXPECT_TRUE(fs::exists(test_dir));
    EXPECT_TRUE(fs::is_directory(test_dir));
}
