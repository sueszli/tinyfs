#include "utils.hpp"
#include <fstream>
#include <gtest/gtest.h>
#include <spdlog/spdlog.h>
#include <vector>

std::shared_ptr<spdlog::logger> logger; // can be null, not in scope of this test suite

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
