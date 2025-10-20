#include "lob/MatchingEngine.h"

namespace lob {

MatchingEngine::MatchingEngine(const EngineConfig& config, 
                               std::shared_ptr<TimeSource> time_source)
    : config_(config)
    , time_source_(time_source ? time_source : std::make_shared<SimulatedTimeSource>())
    , book_(config.tick_size, time_source_)
    , event_buffer_(config.ring_size)
{
}

bool MatchingEngine::submit(const Order& order) {
    std::vector<TradeEvent> trades;
    BookTop top;
    
    bool success = book_.add(order, trades, &top);
    
    if (success) {
        // Emit accept event
        emit_event(AcceptEvent(order.id, time_source_->now_ns()));
        
        // Emit trade events
        for (const auto& trade : trades) {
            emit_event(trade);
        }
        
        // Emit book update
        emit_event(top);
    } else {
        // Emit reject event
        emit_event(RejectEvent(order.id, time_source_->now_ns(), 1));
    }
    
    return success;
}

bool MatchingEngine::cancel(OrderId id) {
    CancelEvent cancel_event;
    bool success = book_.cancel(id, cancel_event);
    
    if (success) {
        emit_event(cancel_event);
        
        // Emit book update
        BookTop top;
        book_.best_bid_ask(top);
        emit_event(top);
    }
    
    return success;
}

bool MatchingEngine::replace(OrderId id, Price new_price, uint64_t new_qty) {
    ReplaceEvent replace_event;
    std::vector<TradeEvent> trades;
    
    bool success = book_.replace(id, new_price, new_qty, replace_event, trades);
    
    if (success) {
        // Emit replace event
        emit_event(replace_event);
        
        // Emit trade events (if matching occurred)
        for (const auto& trade : trades) {
            emit_event(trade);
        }
        
        // Emit book update
        BookTop top;
        book_.best_bid_ask(top);
        emit_event(top);
    }
    
    return success;
}

bool MatchingEngine::poll_events(std::vector<EngineEvent>& out_events) {
    out_events.clear();
    
    EngineEvent event;
    while (event_buffer_.pop(event)) {
        out_events.push_back(event);
    }
    
    return !out_events.empty();
}

void MatchingEngine::emit_event(const EngineEvent& event) {
    // If buffer is full, events will be dropped
    // In production, you might want to handle this differently
    event_buffer_.push(event);
}

} // namespace lob
