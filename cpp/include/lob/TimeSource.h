#pragma once

#include <cstdint>
#include <memory>
#include <chrono>

namespace lob {

// Abstract time source for deterministic simulation
class TimeSource {
public:
    virtual ~TimeSource() = default;
    virtual uint64_t now_ns() noexcept = 0;
};

// Simulated time source with manual control
class SimulatedTimeSource : public TimeSource {
public:
    SimulatedTimeSource(uint64_t initial_ns = 0) noexcept : current_ns_(initial_ns) {}
    
    uint64_t now_ns() noexcept override {
        return current_ns_;
    }
    
    void advance(uint64_t delta_ns) noexcept {
        current_ns_ += delta_ns;
    }
    
    void set(uint64_t ns) noexcept {
        current_ns_ = ns;
    }

private:
    uint64_t current_ns_;
};

// Real-time source using system clock
class RealTimeSource : public TimeSource {
public:
    RealTimeSource() noexcept {
        start_ = std::chrono::steady_clock::now();
    }
    
    uint64_t now_ns() noexcept override {
        auto now = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(now - start_);
        return static_cast<uint64_t>(duration.count());
    }

private:
    std::chrono::steady_clock::time_point start_;
};

} // namespace lob
