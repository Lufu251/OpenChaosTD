#include <core/performance_monitor.hpp>
#include <iostream>
#include <iomanip>
#include <stdexcept>

PerformanceMonitor::PerformanceMonitor(size_t windowSize)
    : m_windowSize(windowSize)
{}

// Begin / End
void PerformanceMonitor::Begin(const std::string& name) {
    m_entries[name].startTime = Clock::now();
}

void PerformanceMonitor::End(const std::string& name) {
    auto it = m_entries.find(name);
    if (it == m_entries.end())
        throw std::runtime_error("PerformanceMonitor: End() called without Begin() for '" + name + "'");

    Entry& entry = it->second;

    double elapsedMs = std::chrono::duration<double, std::milli>(
        Clock::now() - entry.startTime
    ).count();

    entry.samples.push_back(elapsedMs);
    if (entry.samples.size() > m_windowSize)
        entry.samples.pop_front();

    if (elapsedMs > entry.peak)
        entry.peak = elapsedMs;
}

// Results
double PerformanceMonitor::GetAvgMs(const std::string& name) const {
    auto it = m_entries.find(name);
    if (it == m_entries.end() || it->second.samples.empty()) return 0.0;

    const auto& samples = it->second.samples;
    double sum = 0.0;
    for (double s : samples) sum += s;
    return sum / static_cast<double>(samples.size());
}

double PerformanceMonitor::GetLastMs(const std::string& name) const {
    auto it = m_entries.find(name);
    if (it == m_entries.end() || it->second.samples.empty()) return 0.0;
    return it->second.samples.back();
}

double PerformanceMonitor::GetPeakMs(const std::string& name) const {
    auto it = m_entries.find(name);
    if (it == m_entries.end()) return 0.0;
    return it->second.peak;
}

// Print
void PerformanceMonitor::Print() const {
    std::cout << "\n=== PerformanceMonitor ===\n";
    std::cout << std::fixed << std::setprecision(3);
    std::cout << std::left
              << std::setw(16) << "Scope"
              << std::setw(14) << "Avg (ms)"
              << std::setw(14) << "Last (ms)"
              << std::setw(14) << "Peak (ms)"
              << "\n";
    std::cout << std::string(58, '-') << "\n";

    for (const auto& [name, entry] : m_entries) {
        std::cout << std::setw(16) << name
                  << std::setw(14) << GetAvgMs(name)
                  << std::setw(14) << GetLastMs(name)
                  << std::setw(14) << entry.peak
                  << "\n";
    }
    std::cout << "==========================\n\n";
}