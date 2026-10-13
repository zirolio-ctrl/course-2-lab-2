
#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace lab {
    enum class Operation {
        Fast,
        Slow
    };

    struct Measurement {
        std::string method;
        std::size_t size{};
        std::string operation;
        double milliseconds{};
        std::uint64_t checksum{};
        bool isCustom = false;
        bool best = false;
    };

    std::vector<Measurement> benchmark_policies(
        const std::vector<std::uint64_t>& source,
        Operation operation
    );

    std::vector<Measurement> benchmark_custom_threads(
        const std::vector<std::uint64_t>& source,
        Operation operation,
        const std::vector<std::size_t>& thread_counts
    );

    void write_measurements_csv(
        const std::filesystem::path& path,
        const std::vector<Measurement>& measurements
    );

    const char* get_operation_name(Operation operation);

    void print_measurements(const std::vector<lab::Measurement>& measurements);
}