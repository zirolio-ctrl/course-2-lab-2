#include "data.hpp"

#include <fstream>
#include <iostream>
#include <random>
#include <stdexcept>
#include <string>
#include <format>

namespace lab {
    void generate_data_files(
        const std::filesystem::path& directory,
        const std::vector<std::size_t>& sizes,
        const std::uint32_t seed
    ) {
        std::filesystem::create_directories(directory);
        std::mt19937 engine(seed);

        std::uniform_int_distribution<std::uint64_t> distribution(0, 1'000'000);

        for (const std::size_t size : sizes) {
            const auto path = directory / ("data_" + std::to_string(size) + ".txt");

            std::ofstream output(path);

            if (!output) {
                throw std::runtime_error("Cannot open output file: " + path.string());
            }

            output << size << "\n\n";

            for (std::size_t i = 0; i < size; ++i) {
                output << distribution(engine) << '\n';
            }

            std::cout << std::format("Generated {} ({} values)\n", path.string(), size);
        }
    }

    std::vector<std::uint64_t> read_data_file(
        const std::filesystem::path& path
    ) {
        std::ifstream input(path);

        if (!input) {
            throw std::runtime_error("Cannot open input file: " + path.string());
        }

        std::size_t size = 0;
        input >> size;

        std::vector<std::uint64_t> values(size);

        for (auto& value : values) {
            input >> value;
        }

        return values;
    }
}