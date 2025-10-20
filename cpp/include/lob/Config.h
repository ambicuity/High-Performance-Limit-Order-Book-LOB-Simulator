#pragma once

#include <cstddef>

namespace lob {

struct EngineConfig {
    size_t max_orders;      // Maximum number of active orders
    size_t ring_size;       // Size of event ring buffer
    double tick_size;       // Minimum price increment
    
    EngineConfig() noexcept 
        : max_orders(100000), ring_size(10000), tick_size(0.01) {}
    
    EngineConfig(size_t max_ord, size_t ring, double tick) noexcept
        : max_orders(max_ord), ring_size(ring), tick_size(tick) {}
};

} // namespace lob
