#pragma once

#include <expected>
#include <filesystem>
#include <string>
#include <vector>

namespace analysis {

struct MatrixResult {
    std::vector<std::filesystem::path> file_names;
    std::vector<std::vector<int>> correlation_matrix;
};

enum class AnalysisError { DirectoryEmpty, InvalidDirectory, ProcessingFailed };

std::expected<MatrixResult, AnalysisError>
analyze_directory(const std::filesystem::path& dir_path,
                  const std::string& password, size_t sample_points = 512);

} // namespace analysis
