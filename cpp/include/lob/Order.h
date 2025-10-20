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

enum class PegType : uint8_t {
    None = 0,           // Not pegged
    Mid = 1,            // Pegged to mid price
    BestBid = 2,        // Pegged to best bid
    BestAsk = 3         // Pegged to best ask
};

struct Order {
    OrderId id;
    Side side;
    Price price;        // Ignored for pure market orders
    uint64_t qty;
    uint64_t ts;        // Timestamp in nanoseconds
    OrderType type;
    
    // Iceberg order fields
    uint64_t display_qty;   // Visible quantity (0 = show all)
    uint64_t refresh_qty;   // Quantity to refresh on each fill
    
    // Pegged order fields
    PegType peg_type;       // Type of pegging
    int64_t offset;         // Offset in ticks from peg reference

    Order() noexcept 
        : id(INVALID_ORDER_ID), side(Side::Buy), price(), 
          qty(0), ts(0), type(OrderType::Limit),
          display_qty(0), refresh_qty(0), peg_type(PegType::None), offset(0) {}

    Order(OrderId id_, Side side_, Price price_, uint64_t qty_, 
          uint64_t ts_, OrderType type_ = OrderType::Limit) noexcept
        : id(id_), side(side_), price(price_), qty(qty_), ts(ts_), type(type_),
          display_qty(0), refresh_qty(0), peg_type(PegType::None), offset(0) {}

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
    
    [[nodiscard]] bool is_iceberg() const noexcept {
        return display_qty > 0 && display_qty < qty;
    }
    
    [[nodiscard]] bool is_pegged() const noexcept {
        return peg_type != PegType::None;
    }
    
    [[nodiscard]] uint64_t visible_qty() const noexcept {
        return is_iceberg() ? display_qty : qty;
    }
};

} // namespace lob
