#pragma once

#include "Config.h"
#include "LimitBook.h"
#include "Events.h"
#include "TimeSource.h"
#include "RingBuffer.h"
#include <memory>
#include <vector>
#include <variant>

namespace lob {

// Unified event type for all engine events
using EngineEvent = std::variant<TradeEvent, AcceptEvent, RejectEvent, 
                                  CancelEvent, ReplaceEvent, BookTop>;

class MatchingEngine {
public:
    explicit MatchingEngine(const EngineConfig& config, 
                           std::shared_ptr<TimeSource> time_source = nullptr);

    // Submit new order (synchronous API)
    [[nodiscard]] bool submit(const Order& order);

    // Cancel existing order
    [[nodiscard]] bool cancel(OrderId id);

    // Replace existing order (modify price and/or quantity)
    [[nodiscard]] bool replace(OrderId id, Price new_price, uint64_t new_qty);

    // Poll for events from the engine
    [[nodiscard]] bool poll_events(std::vector<EngineEvent>& out_events);

    // Get const reference to order book
    [[nodiscard]] const LimitBook& book() const noexcept {
        return book_;
    }

    // Get best bid/ask
    [[nodiscard]] bool best_bid_ask(BookTop& out) const noexcept {
        return book_.best_bid_ask(out);
    }
    
    // Get market depth snapshot
    void get_depth(DepthSnapshot& out, size_t max_levels = 10) const noexcept {
        book_.get_depth(out, max_levels);
    }

    // Get current timestamp
    [[nodiscard]] uint64_t now() const noexcept {
        return time_source_->now_ns();
    }

    [[nodiscard]] const EngineConfig& config() const noexcept {
        return config_;
    }

private:
    void emit_event(const EngineEvent& event);

    EngineConfig config_;
    std::shared_ptr<TimeSource> time_source_;
    LimitBook book_;
    RingBuffer<EngineEvent> event_buffer_;
};

} // namespace lob
