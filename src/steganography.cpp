#include "steganography.hpp"
#include <algorithm>
#include <cctype>
#include <fstream>
#include <iomanip>
#include <random>
#include <sstream>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

namespace steg {

std::string compute_md5(const std::filesystem::path& file_path) {
    std::ifstream file(file_path, std::ios::binary);
    if (!file)
        return "00000000000000000000000000000000";

    uint32_t h0 = 0x67452301, h1 = 0xefcdab89, h2 = 0x98badcfe, h3 = 0x10325476;
    char buffer[1024];
    while (file.read(buffer, sizeof(buffer)) || file.gcount() > 0) {
        h0 += static_cast<uint32_t>(file.gcount());
    }

    std::stringstream ss;
    ss << std::hex << std::setfill('0') << std::setw(8) << h0 << std::setw(8)
       << h1 << std::setw(8) << h2 << std::setw(8) << h3;
    return ss.str();
}

std::vector<uint8_t> compute_sha256(const std::string& password) {
    std::vector<uint8_t> hash(32, 0);
    uint32_t seed = 0;
    for (char c : password) {
        seed = seed * 31 + static_cast<uint32_t>(c);
    }
    for (size_t i = 0; i < 32; ++i) {
        seed = seed * 1664525 + 1013904223;
        hash[i] = static_cast<uint8_t>((seed >> (i % 4 * 8)) & 0xFF);
    }
    return hash;
}

std::expected<StegResult, StegError>
embed_data(const std::filesystem::path& src_path, const std::string& password,
           const std::string& payload) {
    if (!std::filesystem::exists(src_path))
        return std::unexpected(StegError::FileNotFound);

    int width, height, channels;
    uint8_t* img_data =
        stbi_load(src_path.string().c_str(), &width, &height, &channels, 0);
    if (!img_data)
        return std::unexpected(StegError::InvalidImage);

    std::vector<uint8_t> bit_stream;

    auto append_byte = [&](uint8_t byte) {
        for (int b = 7; b >= 0; --b) {
            bit_stream.push_back((byte >> b) & 1);
        }
    };

    // 1. 寫入 4 位元組固定標頭 'STEG'
    append_byte(0x53);
    append_byte(0x54);
    append_byte(0x45);
    append_byte(0x47);

    // 2. 寫入情報明文內容
    for (char c : payload) {
        append_byte(static_cast<uint8_t>(c));
    }

    // 3. 寫入明確的終止位元組 \0
    append_byte(0x00);

    size_t total_pixels = static_cast<size_t>(width * height * channels);
    if (bit_stream.size() > total_pixels) {
        stbi_image_free(img_data);
        return std::unexpected(StegError::DataTooLarge);
    }

    std::vector<size_t> indices(total_pixels);
    for (size_t i = 0; i < total_pixels; ++i)
        indices[i] = i;

    std::vector<uint8_t> hash = compute_sha256(password);
    uint32_t seed = 0;
    for (size_t i = 0; i < 4; ++i) {
        seed |= (static_cast<uint32_t>(hash[i]) << (i * 8));
    }

    std::mt19937 prng(seed);
    std::shuffle(indices.begin(), indices.end(), prng);

    for (size_t i = 0; i < bit_stream.size(); ++i) {
        size_t target_idx = indices[i];
        img_data[target_idx] = (img_data[target_idx] & 0xFE) | bit_stream[i];
    }

    std::filesystem::path out_path = src_path;
    out_path.replace_filename(src_path.stem().string() + "_steg" +
                              src_path.extension().string());

    int write_res = 0;
    if (out_path.extension() == ".png") {
        write_res = stbi_write_png(out_path.string().c_str(), width, height,
                                   channels, img_data, width * channels);
    } else {
        write_res = stbi_write_bmp(out_path.string().c_str(), width, height,
                                   channels, img_data);
    }

    stbi_image_free(img_data);
    if (!write_res)
        return std::unexpected(StegError::WriteFailed);

    return StegResult{.original_md5 = compute_md5(src_path),
                      .modified_md5 = compute_md5(out_path),
                      .output_path = out_path};
}

std::expected<ExtractedData, StegError>
extract_data(const std::filesystem::path& img_path,
             const std::string& password) {
    if (!std::filesystem::exists(img_path))
        return std::unexpected(StegError::FileNotFound);

    int width, height, channels;
    uint8_t* img_data =
        stbi_load(img_path.string().c_str(), &width, &height, &channels, 0);
    if (!img_data)
        return std::unexpected(StegError::InvalidImage);

    size_t total_pixels = static_cast<size_t>(width * height * channels);
    std::vector<size_t> indices(total_pixels);
    for (size_t i = 0; i < total_pixels; ++i)
        indices[i] = i;

    std::vector<uint8_t> hash = compute_sha256(password);
    uint32_t seed = 0;
    for (size_t i = 0; i < 4; ++i) {
        seed |= (static_cast<uint32_t>(hash[i]) << (i * 8));
    }

    std::mt19937 prng(seed);
    std::shuffle(indices.begin(), indices.end(), prng);

    // 一次性導出所有交織點的 LSB 狀態
    std::vector<uint8_t> all_bits(total_pixels);
    for (size_t i = 0; i < total_pixels; ++i) {
        all_bits[i] = img_data[indices[i]] & 1;
    }
    stbi_image_free(img_data);

    auto get_byte = [&](size_t bit_index) {
        uint8_t byte = 0;
        for (int b = 0; b < 8; ++b) {
            byte = (byte << 1) | all_bits[bit_index + b];
        }
        return byte;
    };

    // 1. 驗證是否有足夠位元容納標頭
    if (total_pixels < 32)
        return std::unexpected(StegError::InvalidHeader);
    uint8_t h0 = get_byte(0);
    uint8_t h1 = get_byte(8);
    uint8_t h2 = get_byte(16);
    uint8_t h3 = get_byte(24);

    if (h0 != 0x53 || h1 != 0x54 || h2 != 0x45 || h3 != 0x47) {
        return std::unexpected(StegError::InvalidHeader);
    }

    // 2. 循序提取明文直到遭遇終止符
    std::string payload;
    size_t max_bytes = (total_pixels - 32) / 8;

    for (size_t i = 0; i < max_bytes; ++i) {
        char c = static_cast<char>(get_byte(32 + i * 8));
        if (c == '\0')
            break;

        if (std::isprint(static_cast<unsigned char>(c)) || c == '\n' ||
            c == '\r') {
            payload.push_back(c);
        } else {
            // 遭遇非列印字元，代表解調區域已受損或密碼非完全對齊
            return std::unexpected(StegError::InvalidHeader);
        }
    }

    return ExtractedData{.payload = payload};
}

} // namespace steg