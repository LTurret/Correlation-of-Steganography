#pragma once

#include <expected>
#include <filesystem>
#include <string>
#include <vector>

namespace steg {

struct StegResult {
    std::string original_md5;
    std::string modified_md5;
    std::filesystem::path output_path;
};

// 精簡後的結構體，移除了 username
struct ExtractedData {
    std::string payload;
};

enum class StegError {
    FileNotFound,
    InvalidImage,
    DataTooLarge,
    InvalidHeader,
    WriteFailed
};

std::string compute_md5(const std::filesystem::path& file_path);
std::vector<uint8_t> compute_sha256(const std::string& password);

// 移除 username 參數
std::expected<StegResult, StegError>
embed_data(const std::filesystem::path& src_path, const std::string& password,
           const std::string& payload);

std::expected<ExtractedData, StegError>
extract_data(const std::filesystem::path& img_path,
             const std::string& password);

} // namespace steg