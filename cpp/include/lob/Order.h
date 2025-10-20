#pragma once

#include "OrderId.h"
#include "Price.h"
#include "Side.h"
#include <cstdint>

namespace lob {

enum class OrderType : uint8_t {
    Limit = 0,
    Market = 1,
    IOC = 2,    // Immediate-Or-Cancel
    FOK = 3     // Fill-Or-Kill
};

struct Order {
    OrderId id;
    Side side;
    Price price;        // Ignored for pure market orders
    uint64_t qty;
    uint64_t ts;        // Timestamp in nanoseconds
    OrderType type;

    Order() noexcept 
        : id(INVALID_ORDER_ID), side(Side::Buy), price(), 
          qty(0), ts(0), type(OrderType::Limit) {}

    Order(OrderId id_, Side side_, Price price_, uint64_t qty_, 
          uint64_t ts_, OrderType type_ = OrderType::Limit) noexcept
        : id(id_), side(side_), price(price_), qty(qty_), ts(ts_), type(type_) {}

    [[nodiscard]] bool is_market() const noexcept {
        return type == OrderType::Market;
    }

    [[nodiscard]] bool is_limit() const noexcept {
        return type == OrderType::Limit;
    }

    [[nodiscard]] bool is_ioc() const noexcept {
        return type == OrderType::IOC;
    }

    [[nodiscard]] bool is_fok() const noexcept {
        return type == OrderType::FOK;
    }
};

} // namespace lob
