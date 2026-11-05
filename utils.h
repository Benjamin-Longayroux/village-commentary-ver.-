#ifndef UTILS_H
#define UTILS_H

#include <iostream>
#include <string>
#include <stdexcept>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <map>
#include <array>
#include <deque>
#include <utility>
#include <random>

void print_student_details();

void help();

void progress_log(const std::string& message);

void debug_log(bool enabled, const std::string& message);

int parse_integer(const std::string& value);

// Collect performance measurements only when --profile is enabled.
// The single-threaded generator uses one process-wide profiler to avoid
// changing Task A / Task B function interfaces during instrumentation.
class PerformanceProfile {
public:
    using Clock = std::chrono::steady_clock;
    struct TimingStats {
        std::uint64_t calls = 0;
        double inclusive_ms = 0.0;
        double exclusive_ms = 0.0;
        double min_ms = 0.0;
        double max_ms = 0.0;
        std::uint64_t slow_50ms = 0;
        std::uint64_t slow_200ms = 0;
        std::array<std::uint64_t, 12> latency_buckets{};
    };

    void begin(int seed, int village_size, int density,
               int loc_x, int loc_z);
    bool enabled() const;
    void add(const std::string& metric, std::uint64_t amount = 1);
    void record_time(const std::string& stage, double milliseconds);
    void record_scope(const char* name, double inclusive_ms,
                      double exclusive_ms, bool is_api);
    void print_summary() const;
    bool append_csv(const std::string& filename, std::size_t plots,
                    std::size_t waypoints, int exit_code) const;

private:
    bool enabled_ = false;
    std::int64_t run_id_ = 0;
    int seed_ = 0;
    int village_size_ = 0;
    int density_ = 0;
    int loc_x_ = 0;
    int loc_z_ = 0;
    std::map<std::string, std::uint64_t> counters_;
    std::map<std::string, TimingStats> scopes_;
    std::map<std::string, TimingStats> api_;
    std::deque<Clock::time_point> api_recent_;
    std::uint64_t api_peak_1s_ = 0;
};

PerformanceProfile& performance_profile();

// Nested scope tracking: inclusive = whole function; exclusive = own work
// without directly nested timers. Profiling never sleeps or throttles calls.
class PerformanceStage {
public:
    explicit PerformanceStage(const char* stage, bool is_api = false);
    ~PerformanceStage();
    PerformanceStage(const PerformanceStage&) = delete;
    PerformanceStage& operator=(const PerformanceStage&) = delete;

private:
    const char* stage_;
    bool active_;
    bool is_api_;
    double child_ms_ = 0.0;
    PerformanceStage* parent_ = nullptr;
    PerformanceProfile::Clock::time_point start_;
    static PerformanceStage* current_;
};

// Measures only the synchronous C++ API call. doCommand may queue work on
// ELCI; return time is NOT proof that the server finished placing blocks.
template <typename Function>
decltype(auto) profiled_mcpp(const char* metric, Function&& operation) {
    if (!performance_profile().enabled()) {
        return std::forward<Function>(operation)();
    }
    PerformanceStage timing(metric, true);
    return std::forward<Function>(operation)();
}

// Argument construction occurs BEFORE this timer starts. This separates
// MinecraftConnection::doCommand latency from local command formatting.
template <typename Connection, typename Text>
decltype(auto) profiled_command(Connection& mc, Text&& command) {
    return profiled_mcpp("mcpp_do_command", [&]() {
        return mc.doCommand(std::forward<Text>(command));
    });
}

int random_int(std::mt19937& rng, int min, int max);

long long random_int(std::mt19937& rng, long long min, long long max);

#endif
