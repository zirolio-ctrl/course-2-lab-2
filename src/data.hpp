#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <vector>

namespace lab {
    void generate_data_files(
        const std::filesystem::path& directory,
        const std::vector<std::size_t>& sizes,
        std::uint32_t seed = 12345678
    );

    std::vector<std::uint64_t> read_data_file(
        const std::filesystem::path& path
    );

}