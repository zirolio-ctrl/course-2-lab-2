// Compiler: GCC or Clang with C++23 support.
// Build mode: Release. Tested project configuration: CMake.
// Optimization: -O0 or -O3, selected through OPT_LEVEL.

#include "benchmark.hpp"
#include "data.hpp"
#include "color.hpp"

#include <algorithm>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <format>
#include <chrono>

using Clock = std::chrono::steady_clock;

const std::filesystem::path data_directory = "data";
const std::filesystem::path results_directory = "results";

const std::vector<std::size_t> data_sizes{
    100'000,
    500'000,
    1'000'000,
    10'000'000,
};

namespace lab {
    std::vector<std::size_t> get_thread_counts() {
        const unsigned int detected = std::thread::hardware_concurrency();
        const std::size_t hardware_threads = detected == 0 ? 4 : detected;
    
        std::vector<std::size_t> counts{ 1, 2, 4, 8, 16, hardware_threads };
    
        std::sort(counts.begin(), counts.end());
    
        counts.erase(
            std::unique(counts.begin(), counts.end()),
            counts.end()
        );
    
        return counts;
    }
    
    void run_benchmarks() {
        const auto thread_counts = get_thread_counts();
        const unsigned int detected = std::thread::hardware_concurrency();
    
        std::cout << "Logical processors reported by hardware_concurrency(): " << detected << '\n';
    
        std::vector<lab::Measurement> all_results;
    
        for (const std::size_t size : data_sizes) {
            const auto path = data_directory / ("data_" + std::to_string(size) + ".txt");
    
            if (!std::filesystem::exists(path)) {
                std::cerr << std::format("Missing {}. Run --generate first.\n", path.string());
                return;
            }
    
            const auto source = lab::read_data_file(path);
    
            for (const auto operation : { lab::Operation::Fast, lab::Operation::Slow }) {
                auto policy_results = lab::benchmark_policies(source, operation);
                auto custom_results = lab::benchmark_custom_threads(source, operation, thread_counts);

                const auto best = std::min_element(
                    custom_results.begin(),
                    custom_results.end(),
                    [](const auto& a, const auto& b) {
                        return a.milliseconds < b.milliseconds;
                    }
                );
    
                if (best != custom_results.end()) {
                    best->best = true;
                }
    
                all_results.insert(
                    all_results.end(),
                    policy_results.begin(),
                    policy_results.end()
                );
    
                all_results.insert(
                    all_results.end(),
                    custom_results.begin(),
                    custom_results.end()
                );
            }
        }

        std::cout << '\n';
        lab::print_measurements(all_results);
    
        lab::write_measurements_csv(results_directory / std::format(
            "benchmark_results_{:%Y-%m-%d_%H-%M-%S}.csv",
            std::chrono::system_clock::now()
        ), all_results);
    
        std::cout << "\nSaved results to results/benchmark_results.csv\n";
    }
}

int main(const int argc, char* argv[]) {
    const std::string mode = argc > 1 ? argv[1] : "--help";
    std::uint32_t seed = 12345678;

    if (mode == "--generate") {
        if (argc > 2) {
            const std::string_view value = argv[2];
            
            const auto [ptr, ec] = std::from_chars(
                value.data(),
                value.data() + value.size(),
                seed
            );
            
            if (ec != std::errc{} || ptr != value.data() + value.size()) {
                std::cerr << "Error: invalid seed: " << value << '\n';
                return 1;
            }
        }
        
        lab::generate_data_files(data_directory, data_sizes, seed);
        return 0;
    }

    if (mode == "--benchmark") {
        lab::run_benchmarks();
        return 0;
    }

    std::cout << std::format(
        "Usage: {} [--generate [seed]|--benchmark|--help]\n",
        argv[0]
    );

    return mode == "--help" ? 0 : 1;
}