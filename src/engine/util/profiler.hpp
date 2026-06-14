#pragma once

#include <chrono>
#include <string>
#include <deque>
#include <unordered_map>

class Profiler {
public:
    explicit Profiler(size_t windowSize = 120); // 120 = 2s at 60fps

    // Manual begin/end
    void Begin(const std::string& name);
    void End(const std::string& name);

    // Prints all tracked scopes to stdout
    void Print() const;

private:
    // Per-scope results, read by Print().
    double GetAvgMs(const std::string& name)  const;
    double GetLastMs(const std::string& name) const;

    using Clock     = std::chrono::high_resolution_clock;
    using TimePoint = std::chrono::time_point<Clock>;

    struct Entry {
        TimePoint          startTime;
        std::deque<double> samples;   // rolling window in ms
        double             peak = 0.0;
    };

    size_t                              m_windowSize;
    std::unordered_map<std::string, Entry> m_entries;
};