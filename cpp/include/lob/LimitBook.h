#pragma once

#include "Order.h"
#include "Events.h"
#include "BookLevel.h"
#include "TimeSource.h"
#include <unordered_map>
#include <map>
#include <vector>
#include <memory>

namespace lob {

class LimitBook {
public:
    explicit LimitBook(double tick_size, std::shared_ptr<TimeSource> time_source);

    // Add order, potentially matching, returns trades and book top
    [[nodiscard]] bool add(const Order& order, std::vector<TradeEvent>& out_trades, 
                           BookTop* out_top = nullptr);

    // Cancel order by ID
    [[nodiscard]] bool cancel(OrderId id, CancelEvent& out);

    // Replace order (cancel and re-add with new price/qty)
    [[nodiscard]] bool replace(OrderId id, Price new_price, uint64_t new_qty, 
                                ReplaceEvent& out, std::vector<TradeEvent>& out_trades);

    // Get best bid/ask snapshot
    [[nodiscard]] bool best_bid_ask(BookTop& out) const noexcept;
    
    // Get market depth snapshot up to specified levels
    void get_depth(DepthSnapshot& out, size_t max_levels = 10) const noexcept;

    // Get total number of active orders
    [[nodiscard]] size_t total_orders() const noexcept {
        return order_index_.size();
    }

    [[nodiscard]] double tick_size() const noexcept {
        return tick_size_;
    }

private:
    // Match order against opposite side, generating trades
    void match_order(Order& order, std::vector<TradeEvent>& out_trades);
    
    // Add resting order to book (after matching or if no match)
    void add_resting_order(const Order& order);
    
    // Remove price level if empty
    void cleanup_empty_level(Side side, Price price);

    // Get best price for side
    [[nodiscard]] Price best_price(Side side) const noexcept;

    // Check if order would cross (aggressive)
    [[nodiscard]] bool would_cross(const Order& order) const noexcept;

    double tick_size_;
    std::shared_ptr<TimeSource> time_source_;
    
    // Price -> BookLevel maps (buy side descending, sell side ascending)
    std::map<Price, BookLevel, std::greater<Price>> bids_;  // Descending
    std::map<Price, BookLevel, std::less<Price>> asks_;     // Ascending
    
    // OrderId -> (Side, Price) for quick lookups
    struct OrderLocation {
        Side side;
        Price price;
    };
    std::unordered_map<OrderId, OrderLocation> order_index_;
};

} // namespace lob
