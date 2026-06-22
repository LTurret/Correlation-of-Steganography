#include "matrix_analysis.hpp"

#include <algorithm>
#include <random>

#include "stb_image.h"
#include "steganography.hpp"

namespace analysis {

std::expected<MatrixResult, AnalysisError>
analyze_directory(const std::filesystem::path& dir_path,
                  const std::string& password, size_t sample_points) {
    if (!std::filesystem::exists(dir_path) ||
        !std::filesystem::is_directory(dir_path)) {
        return std::unexpected(AnalysisError::InvalidDirectory);
    }

    std::vector<std::filesystem::path> files;
    for (const auto& entry : std::filesystem::directory_iterator(dir_path)) {
        auto ext = entry.path().extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        if (ext == ".png" || ext == ".bmp") {
            files.push_back(entry.path());
        }
    }

    if (files.empty())
        return std::unexpected(AnalysisError::DirectoryEmpty);

    size_t m = files.size();
    std::vector<std::vector<int>> A(m, std::vector<int>(sample_points, 0));

    std::vector<uint8_t> hash = steg::compute_sha256(password);
    uint32_t seed = 0;
    for (size_t i = 0; i < 4; ++i) {
        seed |= (static_cast<uint32_t>(hash[i]) << (i * 8));
    }

    for (size_t i = 0; i < m; ++i) {
        int width, height, channels;
        uint8_t* img_data =
            stbi_load(files[i].string().c_str(), &width, &height, &channels, 0);
        if (!img_data)
            continue;

        size_t total_pixels = static_cast<size_t>(width * height * channels);
        std::vector<size_t> indices(total_pixels);
        for (size_t k = 0; k < total_pixels; ++k)
            indices[k] = k;

        std::mt19937 prng(seed);
        std::shuffle(indices.begin(), indices.end(), prng);

        for (size_t j = 0; j < sample_points && j < total_pixels; ++j) {
            A[i][j] = img_data[indices[j]] & 1;
        }
        stbi_image_free(img_data);
    }

    std::vector<std::vector<int>> C(m, std::vector<int>(m, 0));
    for (size_t i = 0; i < m; ++i) {
        for (size_t j = 0; j < m; ++j) {
            int sum = 0;
            for (size_t k = 0; k < sample_points; ++k) {
                sum += A[i][k] * A[j][k];
            }
            C[i][j] = sum;
        }
    }

    return MatrixResult{.file_names = files, .correlation_matrix = C};
}

} // namespace analysis
