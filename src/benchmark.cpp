#include "benchmark.hpp"
#include "color.hpp"

#include <algorithm>
#include <chrono>
#include <execution>
#include <fstream>
#include <numeric>
#include <thread>
#include <vector>
#include <string>
#include <iostream>

using Clock = std::chrono::steady_clock;

namespace lab {
    namespace {
        std::uint64_t apply_operation(std::uint64_t value, const Operation operation) {
            if (operation == Operation::Fast) {
                return value + 1;
            }

            std::uint64_t x = value + 1;

            for (int i = 0; i < 60; ++i) {
                x = x * 1'664'525ULL + 1'013'904'223ULL;
                x ^= (x >> 13);
            }

            return x;
        }

        std::uint64_t checksum(const std::vector<std::uint64_t>& values) {
            return std::accumulate(
                values.begin(),
                values.end(),
                std::uint64_t{0}
            );
        }

        template <class Policy> Measurement measure_policy(
            const std::vector<std::uint64_t>& source,
            const Operation operation,
            const char* method,
            Policy policy
        ) {
            auto values = source;

            const auto start = Clock::now();

            std::for_each(
                policy,
                values.begin(),
                values.end(),
                [operation](std::uint64_t& value) {
                    value = apply_operation(value, operation);
                }
            );

            const auto stop = Clock::now();

            return {
                method,
                source.size(),
                get_operation_name(operation),
                std::chrono::duration<double, std::milli>(stop - start).count(),
                checksum(values)
            };
        }

        Measurement measure_no_policy(
            const std::vector<std::uint64_t>& source,
            const Operation operation
        ) {
            auto values = source;

            const auto start = Clock::now();

            std::for_each(
                values.begin(),
                values.end(),
                [operation](std::uint64_t& value) {
                    value = apply_operation(value, operation);
                }
            );

            const auto stop = Clock::now();

            return {
                "no_policy",
                source.size(),
                get_operation_name(operation),
                std::chrono::duration<double, std::milli>(stop - start).count(),
                checksum(values)
            };
        }
    }

    const char* get_operation_name(const Operation operation) {
        return operation == Operation::Fast ? "fast" : "slow";
    }

    std::vector<Measurement> benchmark_policies(
        const std::vector<std::uint64_t>& source,
        const Operation operation
    ) {
        std::vector<Measurement> result;
        result.reserve(4);

        result.push_back(
            measure_no_policy(source, operation)
        );

        result.push_back(
            measure_policy(source, operation, "seq", std::execution::seq)
        );

        result.push_back(
            measure_policy(source, operation, "par", std::execution::par)
        );

        result.push_back(
            measure_policy(source, operation, "par_unseq", std::execution::par_unseq)
        );

        return result;
    }

    std::vector<Measurement> benchmark_custom_threads(
        const std::vector<std::uint64_t>& source,
        Operation operation,
        const std::vector<std::size_t>& thread_counts
    ) {
        std::vector<Measurement> result;
        result.reserve(thread_counts.size());

        for (const std::size_t requested_k : thread_counts) {
            if (source.empty()) {
                result.push_back({
                    "custom_K=" + std::to_string(requested_k),
                    0,
                    get_operation_name(operation),
                    0.0,
                    0,
                    true
                });
                continue;
            }

            const std::size_t k = std::max<std::size_t>(
                1,
                std::min(requested_k, source.size())
            );

            auto values = source;

            std::vector<std::uint64_t> partial_checksums(k, 0);
            std::vector<std::thread> workers;
            workers.reserve(k);

            const auto start = Clock::now();

            for (std::size_t thread_index = 0; thread_index < k; thread_index++) {
                const std::size_t begin = source.size() * thread_index / k;
                const std::size_t end = source.size() * (thread_index + 1) / k;

                workers.emplace_back(
                    [&, begin, end, thread_index, operation] {
                        auto first = values.begin() + static_cast<std::ptrdiff_t>(begin);
                        auto last = values.begin() + static_cast<std::ptrdiff_t>(end);

                        std::uint64_t local_sum = 0;

                        std::for_each(
                            first,
                            last,
                            [operation, &local_sum](
                                std::uint64_t& value
                            ) {
                                value = apply_operation(value, operation);
                                local_sum += value;
                            }
                        );

                        partial_checksums[thread_index] = local_sum;
                    }
                );
            }

            for (auto& worker : workers) {
                worker.join();
            }

            std::uint64_t total_checksum = 0;

            std::for_each(
                partial_checksums.begin(),
                partial_checksums.end(),
                [&total_checksum](const std::uint64_t part) {
                    total_checksum += part;
                }
            );

            const auto stop = Clock::now();

            result.push_back({
                "custom_K=" + std::to_string(k),
                source.size(),
                get_operation_name(operation),
                std::chrono::duration<double, std::milli>(stop - start).count(),
                total_checksum,
                true
            });
        }

        return result;
    }

    void write_measurements_csv(
        const std::filesystem::path& path,
        const std::vector<Measurement>& measurements
    ) {
        if (!path.parent_path().empty()) {
            std::filesystem::create_directories(
                path.parent_path()
            );
        }

        std::ofstream output(path);

        output << "method,size,operation,milliseconds,checksum\n";

        for (const auto& m : measurements) {
            output  << m.method << ','
                    << m.size << ','
                    << m.operation << ','
                    << m.milliseconds << ','
                    << m.checksum << '\n';
        }
    }

    void print_measurements(
        const std::vector<lab::Measurement>& measurements
    ) {
        constexpr int method_width = 22;
        constexpr int size_width = 12;
        constexpr int operation_width = 12;
        constexpr int time_width = 14;
        constexpr int checksum_width = 26;

        const int total_width =
            method_width + size_width + operation_width
            + time_width + checksum_width + (5 * 2 - 2);

        const std::string divider(total_width, '-');

        std::cout << Color::Dim << "+" << divider << "+" << Color::Reset << '\n';

        std::cout
            << Color::Dim << "| " << Color::Reset
            << Color::Bold << Color::Cyan << std::left
            << std::setw(method_width) << "Method" << "| "
            << std::right
            << std::setw(size_width - 1) << "N" << " | "
            << std::setw(operation_width - 1) << "Operation" << " | "
            << std::setw(time_width - 1) << "Time (ms)" << " | "
            << std::left
            << std::setw(checksum_width - 1) << "Checksum"
            << Color::Reset << Color::Dim << "|" << Color::Reset << '\n';

        std::cout << Color::Dim << "+" << divider << "+" << Color::Reset << '\n';

        for (std::size_t i = 0; i < measurements.size(); ++i) {
            const auto& m = measurements[i];

            if (i > 0) {
                const auto& prev = measurements[i - 1];

                if (prev.operation != m.operation ||
                    prev.size != m.size) {
                    std::cout << Color::Dim << "+" << divider << "+" << Color::Reset << '\n';
                }
            }

            const char* row_color = Color::White;

            if (m.best) row_color = Color::Green;
            else if (m.isCustom) row_color = Color::Magenta;
            
            const std::string marker = m.best ? " *" : "";

            std::cout << Color::Dim << "| " << Color::Reset << row_color
                << std::left
                << std::setw(method_width) << (m.method + marker) << "| "
                << std::right
                << std::setw(size_width - 1) << m.size << " | "
                << std::setw(operation_width - 1) << m.operation << " | ";

            if (m.best) std::cout << Color::Bold;

            std::cout
                << std::setw(time_width - 1)
                << std::fixed << std::setprecision(3)
                << m.milliseconds;

            std::cout << Color::Reset << row_color << " | "
                << std::left
                << std::setw(checksum_width - 1)
                << m.checksum
                << Color::Reset << Color::Dim << "|" << Color::Reset << '\n';
        }

        std::cout << Color::Dim << "+" << divider << "+" << Color::Reset << '\n';

        std::cout << Color::Green << Color::Bold
                << "* Best measured result" << Color::Reset << '\n';

        std::cout << Color::Magenta
                << "Custom multithreaded algorithm" << Color::Reset << '\n';
    }
}