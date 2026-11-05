#include <fstream>
#include <iomanip>
#include <limits>

#include "utils.h"

void print_student_details() {
    std::cout << "Student 1 - Task A: Josh A - s4246434 - s4246434"
              << std::endl;
    std::cout << "Student 2 - Task B: Ben L - s4246540 - Benjamin-Longayroux"
              << std::endl;
}

void help() {
    std::cout << "Minecraft Village Generator\n\n";

    std::cout << "Usage: ./gen-village [OPTIONS]\n\n";

    std::cout << "Options:\n";
    std::cout << "  --loc=<x>,<z>        Village centre "
                 "(default: player position)\n";
    std::cout << "  --village-size=<int> Village size (default: 200)\n";
    std::cout << "  --density=<int>      Village density (default: 3)\n";
    std::cout << "  --seed=<int>         Random seed (default: current time)\n";
    std::cout << "  --debug              Enable verbose diagnostic output\n";
    std::cout << "  --profile            Show timings and append "
              << "testing/performance.csv\n";
    std::cout << "  --help               Display this help message\n";
}

void progress_log(const std::string& message) {
    // Flush each major milestone so long Minecraft operations show progress.
    std::cout << "[STAGE] " << message << std::endl;
}

void debug_log(bool enabled, const std::string& message) {
    // Keep diagnostics in order with stage updates on stdout.
    if (enabled) {
        std::cout << "[DEBUG] " << message << '\n';
    }
}

int parse_integer(const std::string& value) {
    std::size_t position = 0;
    int result = std::stoi(value, &position);

    if (position != value.size()) {
        throw std::invalid_argument("Invalid integer format");
    }

    return result;
}

long long random_int(std::mt19937& rng, long long min, long long max) {
    static_assert(std::numeric_limits<int>::digits == 31,
                  "Village coordinates require 32-bit int");
    static_assert(std::numeric_limits<long long>::digits == 63,
                  "Sampler requires 64-bit long long");
    static_assert(std::mt19937::min() == 0 &&
                  std::mt19937::max() == 0xffffffffULL,
                  "Sampler requires 32-bit mt19937 output");
    if (min > max) {
        throw std::invalid_argument("Invalid random range");
    }

    const std::uint64_t span = static_cast<std::uint64_t>(max) -
        static_cast<std::uint64_t>(min);

    const bool wide = span >= (std::uint64_t{1} << 32);

    const std::uint64_t source_max = wide
        ? std::numeric_limits<std::uint64_t>::max() : 0xffffffffULL;

    const bool full_width = span == source_max;

    const std::uint64_t range = full_width ? 0 : span + 1;

    const std::uint64_t rejected = full_width ? 0
        : (source_max % range + 1) % range;

    const std::uint64_t last_accepted = source_max - rejected;

    long long result = 0;
    bool accepted = false;

    for (int attempt = 0; attempt < 128 && !accepted; ++attempt) {
        std::uint64_t value = rng();
        if (wide) {
            value = (value << 32) | static_cast<std::uint64_t>(rng());
        }

        if (value <= last_accepted) {
            const std::uint64_t offset = full_width ? value : value % range;
            if (min < 0) {
                const std::uint64_t to_zero =
                    std::uint64_t{0} - static_cast<std::uint64_t>(min);
                if (offset < to_zero) {
                    result = min + static_cast<long long>(offset);
                } else {
                    result = static_cast<long long>(offset - to_zero);
                }
            } else {
                result = min + static_cast<long long>(offset);
            }
            accepted = true;
        }
    }

    if (!accepted) {
        throw std::runtime_error("Random integer rejection limit exceeded");
    }

    return result;
}

int random_int(std::mt19937& rng, int min, int max) {
    return static_cast<int>(random_int(rng, static_cast<long long>(min),
                                     static_cast<long long>(max)));
}

namespace {
    constexpr std::array<double, 12> kApiLatencyBounds = {
        0.5, 1, 2, 5, 10, 20, 50, 100, 250, 500, 1000, 5000
    };

    double estimated_p95(const PerformanceProfile::TimingStats& stats) {
        double result = 0.0;

        if (stats.calls > 0) {
            const std::uint64_t target = (stats.calls * 95 + 99) / 100;
            std::uint64_t subtotal = 0;
            std::size_t index = 0;

            while (index < kApiLatencyBounds.size() && subtotal < target) {
                subtotal += stats.latency_buckets[index];
                if (subtotal >= target) {
                    result = kApiLatencyBounds[index];
                }
                ++index;
            }

            if (subtotal < target) {
                result = stats.max_ms;
            }
        }

        return result;
    }
}

PerformanceProfile& performance_profile() {
    static PerformanceProfile instance;
    return instance;
}

PerformanceStage* PerformanceStage::current_ = nullptr;

void PerformanceProfile::begin(int seed, int village_size, int density,
                               int loc_x, int loc_z) {
    enabled_ = true;
    seed_ = seed;
    village_size_ = village_size;
    density_ = density;
    loc_x_ = loc_x;
    loc_z_ = loc_z;
    run_id_ = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    counters_.clear();
    scopes_.clear();
    api_.clear();
    api_recent_.clear();
    api_peak_1s_ = 0;
}

bool PerformanceProfile::enabled() const { return enabled_; }

void PerformanceProfile::add(const std::string& metric, std::uint64_t amount) {
    if (enabled_) counters_[metric] += amount;
}

void PerformanceProfile::record_time(const std::string& stage,
                                     double milliseconds) {
    if (enabled_) {
        auto& stats = scopes_[stage];
        ++stats.calls;
        stats.inclusive_ms += milliseconds;
        stats.exclusive_ms += milliseconds;
        if (stats.calls == 1 || milliseconds < stats.min_ms) {
            stats.min_ms = milliseconds;
        }
        if (milliseconds > stats.max_ms) {
            stats.max_ms = milliseconds;
        }
    }
}

void PerformanceProfile::record_scope(const char* name, double inclusive_ms,
                                      double exclusive_ms, bool is_api) {
    if (enabled_) {
        auto update = [inclusive_ms, exclusive_ms](TimingStats& stats) {
            ++stats.calls;
            stats.inclusive_ms += inclusive_ms;
            stats.exclusive_ms += exclusive_ms;
            if (stats.calls == 1 || inclusive_ms < stats.min_ms) {
                stats.min_ms = inclusive_ms;
            }
            if (inclusive_ms > stats.max_ms) {
                stats.max_ms = inclusive_ms;
            }
        };

        if (!is_api) {
            update(scopes_[name]);
        } else {
            auto& stats = api_[name];
            update(stats);

            if (inclusive_ms >= 50) {
                ++stats.slow_50ms;
            }
            if (inclusive_ms >= 200) {
                ++stats.slow_200ms;
            }

            std::size_t index = 0;
            bool bucket_found = false;
            while (index < kApiLatencyBounds.size() && !bucket_found) {
                if (inclusive_ms <= kApiLatencyBounds[index]) {
                    ++stats.latency_buckets[index];
                    bucket_found = true;
                }
                ++index;
            }

            // Track completed API calls over the most recent second.
            const auto now = Clock::now();
            api_recent_.push_back(now);
            while (!api_recent_.empty() &&
                   now - api_recent_.front() >= std::chrono::seconds(1)) {
                api_recent_.pop_front();
            }
            if (api_recent_.size() > api_peak_1s_) {
                api_peak_1s_ = api_recent_.size();
            }
        }
    }
}

PerformanceStage::PerformanceStage(const char* stage, bool is_api)
    : stage_(stage), active_(performance_profile().enabled()), is_api_(is_api) {
    if (active_) {
        parent_ = current_;
        current_ = this;
        start_ = PerformanceProfile::Clock::now();
    }
}

PerformanceStage::~PerformanceStage() {
    if (active_) {
        const double milliseconds = std::chrono::duration<double, std::milli>(
            PerformanceProfile::Clock::now() - start_).count();
        current_ = parent_;
        if (parent_) {
            parent_->child_ms_ += milliseconds;
        }
        const double exclusive_ms = milliseconds > child_ms_
            ? milliseconds - child_ms_ : 0.0;
        performance_profile().record_scope(
            stage_, milliseconds, exclusive_ms, is_api_);
    }
}

void PerformanceProfile::print_summary() const {
    if (enabled_) {
        std::clog << "\n[PROFILE] Stage timings: "
                     "inclusive / exclusive / calls / avg / max (ms)\n";
        for (const auto& entry : scopes_) {
            const auto& stats = entry.second;
            std::clog << "[PROFILE] " << entry.first << " = "
                      << std::fixed << std::setprecision(2)
                      << stats.inclusive_ms << " / " << stats.exclusive_ms
                      << " / " << stats.calls << " / "
                      << (stats.inclusive_ms / stats.calls) << " / "
                      << stats.max_ms
                      << "\n";
        }
        std::clog << "[PROFILE] Minecraft API: "
                     "calls / total-ms / avg-ms / max-ms / "
                     "p95-upper-ms / >=50ms / >=200ms\n";
        for (const auto& entry : api_) {
            const auto& stats = entry.second;
            std::clog << "[PROFILE] " << entry.first << " = " << stats.calls
                      << " / " << stats.inclusive_ms << " / "
                      << (stats.inclusive_ms / stats.calls) << " / "
                      << stats.max_ms << " / " << estimated_p95(stats) << " / "
                      << stats.slow_50ms << " / " << stats.slow_200ms << "\n";
        }
        std::clog << "[PROFILE] peak_completed_api_calls_1s = "
                  << api_peak_1s_ << "\n";
        std::clog << "[PROFILE] Operation counts:\n";
        for (const auto& counter : counters_) {
            std::clog << "[PROFILE] " << counter.first << " = "
                      << counter.second << "\n";
        }
    }
}

bool PerformanceProfile::append_csv(const std::string& filename,
                                    std::size_t plots, std::size_t waypoints,
                                    int exit_code) const {
    bool success = true;
    if (enabled_) {
        std::ifstream existing(filename, std::ios::binary | std::ios::ate);
        const bool header = !existing || existing.tellg() == 0;
        existing.close();
        std::ofstream output(filename, std::ios::app);
        if (!output) {
            success = false;
        }
        if (success) {
            if (header) {
                output << "run_id,seed,village_size,density,loc_x,loc_z,plots,"
                          "waypoints,exit_code,kind,metric,value,unit\n";
            }
            output << std::fixed << std::setprecision(4);
            auto row = [&](const char* kind, const std::string& metric,
                           double value, const char* unit) {
                output << run_id_ << ',' << seed_ << ',' << village_size_ << ','
                       << density_ << ',' << loc_x_ << ',' << loc_z_ << ','
                       << plots << ',' << waypoints << ',' << exit_code << ','
                       << kind << ',' << metric << ',' << value << ','
                       << unit << '\n';
            };

            for (const auto& entry : scopes_) {
                const auto& stats = entry.second;
                // Retain the original CSV metrics for backwards compatibility.
                row("time", entry.first, stats.inclusive_ms, "ms");
                row("time_exclusive", entry.first, stats.exclusive_ms, "ms");
                row("scope_calls", entry.first, stats.calls, "count");
                row("time_max", entry.first, stats.max_ms, "ms");
            }

            for (const auto& entry : api_) {
                const auto& stats = entry.second;
                // Retain the original count metric for backwards compatibility.
                row("count", entry.first, stats.calls, "count");
                row("api_time", entry.first, stats.inclusive_ms, "ms");
                row(
                    "api_avg", entry.first,
                    stats.inclusive_ms / stats.calls, "ms");
                row("api_max", entry.first, stats.max_ms, "ms");
                row("api_p95_upper", entry.first, estimated_p95(stats), "ms");
                row("api_slow_50ms", entry.first, stats.slow_50ms, "count");
                row("api_slow_200ms", entry.first, stats.slow_200ms, "count");
            }

            row("count", "api_peak_completed_calls_1s", api_peak_1s_, "count");

            for (const auto& counter : counters_) {
                row("count", counter.first, counter.second, "count");
            }
            success = output.good();
        }
    }
    return success;
}