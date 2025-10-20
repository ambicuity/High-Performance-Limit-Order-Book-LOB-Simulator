#pragma once

#include "OrderId.h"
#include "Price.h"
#include <cstdint>

namespace lob {

enum class EventType : uint8_t {
    Trade = 0,
    OrderAccepted = 1,
    OrderRejected = 2,
    OrderCanceled = 3,
    OrderReplaced = 4,
    BookUpdate = 5
};

struct TradeEvent {
    EventType type = EventType::Trade;
    OrderId taker_id;
    OrderId maker_id;
    Price price;
    uint64_t qty;
    uint64_t ts;

    TradeEvent() noexcept : taker_id(0), maker_id(0), price(), qty(0), ts(0) {}
    TradeEvent(OrderId taker, OrderId maker, Price p, uint64_t q, uint64_t t) noexcept
        : taker_id(taker), maker_id(maker), price(p), qty(q), ts(t) {}
};

struct AcceptEvent {
    EventType type = EventType::OrderAccepted;
    OrderId id;
    uint64_t ts;

    AcceptEvent() noexcept : id(0), ts(0) {}
    AcceptEvent(OrderId order_id, uint64_t timestamp) noexcept
        : id(order_id), ts(timestamp) {}
};

struct RejectEvent {
    EventType type = EventType::OrderRejected;
    OrderId id;
    uint64_t ts;
    uint32_t reason_code;

    RejectEvent() noexcept : id(0), ts(0), reason_code(0) {}
    RejectEvent(OrderId order_id, uint64_t timestamp, uint32_t reason) noexcept
        : id(order_id), ts(timestamp), reason_code(reason) {}
};

struct CancelEvent {
    EventType type = EventType::OrderCanceled;
    OrderId id;
    uint64_t remaining;
    uint64_t ts;

    CancelEvent() noexcept : id(0), remaining(0), ts(0) {}
    CancelEvent(OrderId order_id, uint64_t rem, uint64_t timestamp) noexcept
        : id(order_id), remaining(rem), ts(timestamp) {}
};

struct ReplaceEvent {
    EventType type = EventType::OrderReplaced;
    OrderId id;
    Price new_price;
    uint64_t new_qty;
    uint64_t ts;

    ReplaceEvent() noexcept : id(0), new_price(), new_qty(0), ts(0) {}
    ReplaceEvent(OrderId order_id, Price px, uint64_t qty, uint64_t timestamp) noexcept
        : id(order_id), new_price(px), new_qty(qty), ts(timestamp) {}
};

struct BookTop {
    EventType type = EventType::BookUpdate;
    Price best_bid;
    uint64_t bid_qty;
    Price best_ask;
    uint64_t ask_qty;
    uint64_t ts;

    BookTop() noexcept 
        : best_bid(INVALID_PRICE), bid_qty(0), 
          best_ask(INVALID_PRICE), ask_qty(0), ts(0) {}
};

} // namespace lob
