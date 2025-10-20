#include "lob/LimitBook.h"
#include <algorithm>

namespace lob {

LimitBook::LimitBook(double tick_size, std::shared_ptr<TimeSource> time_source)
    : tick_size_(tick_size)
    , time_source_(time_source ? time_source : std::make_shared<SimulatedTimeSource>())
{
}

bool LimitBook::add(const Order& order, std::vector<TradeEvent>& out_trades, BookTop* out_top) {
    // Check if order already exists
    if (order_index_.find(order.id) != order_index_.end()) {
        return false; // Duplicate order ID
    }

    Order working_order = order;
    
    // Handle different order types
    if (working_order.is_market() || working_order.is_ioc() || working_order.is_fok()) {
        // For FOK, check if full quantity can be filled
        if (working_order.is_fok()) {
            uint64_t available_qty = 0;
            
            if (working_order.side == Side::Buy) {
                for (auto& [price, level] : asks_) {
                    if (price.ticks > working_order.price.ticks && !working_order.is_market()) {
                        break; // Price too high for buy
                    }
                    
                    available_qty += level.total_qty();
                    if (available_qty >= working_order.qty) {
                        break;
                    }
                }
            } else {
                for (auto& [price, level] : bids_) {
                    if (price.ticks < working_order.price.ticks && !working_order.is_market()) {
                        break; // Price too low for sell
                    }
                    
                    available_qty += level.total_qty();
                    if (available_qty >= working_order.qty) {
                        break;
                    }
                }
            }
            
            if (available_qty < working_order.qty) {
                return false; // FOK rejected - insufficient liquidity
            }
        }
        
        // Match market/IOC/FOK orders
        match_order(working_order, out_trades);
        
        // IOC/FOK don't rest on book
        // Market orders can rest if there's remaining quantity on limit order book
        if (working_order.qty == 0 || working_order.is_ioc() || working_order.is_fok()) {
            if (out_top) {
                best_bid_ask(*out_top);
            }
            return true;
        }
    }
    
    // Try to match limit orders against existing orders
    if (would_cross(working_order)) {
        match_order(working_order, out_trades);
    }
    
    // Add remaining quantity to book if any
    if (working_order.qty > 0 && working_order.is_limit()) {
        add_resting_order(working_order);
    }
    
    if (out_top) {
        best_bid_ask(*out_top);
    }
    
    return true;
}

void LimitBook::match_order(Order& order, std::vector<TradeEvent>& out_trades) {
    if (order.side == Side::Buy) {
        // Match buy order against asks
        while (order.qty > 0 && !asks_.empty()) {
            auto it = asks_.begin();
            Price best_price = it->first;
            BookLevel& level = it->second;
            
            // Check if we can match at this price
            if (!order.is_market()) {
                if (best_price.ticks > order.price.ticks) {
                    break; // Price too high
                }
            }
            
            BookOrder* maker_order = level.front();
            if (!maker_order) {
                break; // Should not happen
            }
            
            // Calculate fill quantity
            uint64_t fill_qty = std::min(order.qty, maker_order->remaining_qty);
            
            // Generate trade event
            TradeEvent trade;
            trade.taker_id = order.id;
            trade.maker_id = maker_order->order.id;
            trade.price = best_price;
            trade.qty = fill_qty;
            trade.ts = time_source_->now_ns();
            out_trades.push_back(trade);
            
            // Update quantities
            order.qty -= fill_qty;
            maker_order->remaining_qty -= fill_qty;
            
            // Remove maker if fully filled
            if (maker_order->remaining_qty == 0) {
                OrderId filled_id = maker_order->order.id;
                level.pop_front();
                order_index_.erase(filled_id);
                
                // Clean up empty level
                if (level.empty()) {
                    asks_.erase(it);
                }
            } else {
                // Update level quantity
                level.update_front_qty(maker_order->remaining_qty);
            }
        }
    } else {
        // Match sell order against bids
        while (order.qty > 0 && !bids_.empty()) {
            auto it = bids_.begin();
            Price best_price = it->first;
            BookLevel& level = it->second;
            
            // Check if we can match at this price
            if (!order.is_market()) {
                if (best_price.ticks < order.price.ticks) {
                    break; // Price too low
                }
            }
            
            BookOrder* maker_order = level.front();
            if (!maker_order) {
                break; // Should not happen
            }
            
            // Calculate fill quantity
            uint64_t fill_qty = std::min(order.qty, maker_order->remaining_qty);
            
            // Generate trade event
            TradeEvent trade;
            trade.taker_id = order.id;
            trade.maker_id = maker_order->order.id;
            trade.price = best_price;
            trade.qty = fill_qty;
            trade.ts = time_source_->now_ns();
            out_trades.push_back(trade);
            
            // Update quantities
            order.qty -= fill_qty;
            maker_order->remaining_qty -= fill_qty;
            
            // Remove maker if fully filled
            if (maker_order->remaining_qty == 0) {
                OrderId filled_id = maker_order->order.id;
                level.pop_front();
                order_index_.erase(filled_id);
                
                // Clean up empty level
                if (level.empty()) {
                    bids_.erase(it);
                }
            } else {
                // Update level quantity
                level.update_front_qty(maker_order->remaining_qty);
            }
        }
    }
}

void LimitBook::add_resting_order(const Order& order) {
    BookOrder book_order(order);
    
    // Add to appropriate side
    if (order.side == Side::Buy) {
        bids_[order.price].add_order(book_order);
    } else {
        asks_[order.price].add_order(book_order);
    }
    
    // Index the order
    order_index_[order.id] = {order.side, order.price};
}

bool LimitBook::cancel(OrderId id, CancelEvent& out) {
    auto it = order_index_.find(id);
    if (it == order_index_.end()) {
        return false; // Order not found
    }
    
    const OrderLocation& loc = it->second;
    uint64_t removed_qty = 0;
    
    // Remove from appropriate side
    if (loc.side == Side::Buy) {
        auto level_it = bids_.find(loc.price);
        if (level_it != bids_.end()) {
            level_it->second.remove_order(id, removed_qty);
            if (level_it->second.empty()) {
                bids_.erase(level_it);
            }
        }
    } else {
        auto level_it = asks_.find(loc.price);
        if (level_it != asks_.end()) {
            level_it->second.remove_order(id, removed_qty);
            if (level_it->second.empty()) {
                asks_.erase(level_it);
            }
        }
    }
    
    // Remove from index
    order_index_.erase(it);
    
    // Fill output
    out.id = id;
    out.remaining = removed_qty;
    out.ts = time_source_->now_ns();
    
    return true;
}

bool LimitBook::replace(OrderId id, Price new_price, uint64_t new_qty,
                        ReplaceEvent& out, std::vector<TradeEvent>& out_trades) {
    auto it = order_index_.find(id);
    if (it == order_index_.end()) {
        return false; // Order not found
    }
    
    const OrderLocation& loc = it->second;
    
    // Find the order
    BookOrder* book_order = nullptr;
    if (loc.side == Side::Buy) {
        auto level_it = bids_.find(loc.price);
        if (level_it != bids_.end()) {
            book_order = level_it->second.find_order(id);
        }
    } else {
        auto level_it = asks_.find(loc.price);
        if (level_it != asks_.end()) {
            book_order = level_it->second.find_order(id);
        }
    }
    
    if (!book_order) {
        return false; // Order not found in level
    }
    
    // Save original order details
    Order original_order = book_order->order;
    original_order.qty = book_order->remaining_qty;
    
    // Cancel the original order
    CancelEvent cancel_event;
    if (!cancel(id, cancel_event)) {
        return false;
    }
    
    // Create new order with replacement details
    Order new_order = original_order;
    new_order.price = new_price;
    new_order.qty = new_qty;
    new_order.ts = time_source_->now_ns(); // Update timestamp (loses time priority)
    
    // Add the new order
    if (!add(new_order, out_trades)) {
        // If add fails, we've lost the original order - this is intentional
        // In production, you might want to handle this differently
        return false;
    }
    
    // Fill output
    out.id = id;
    out.new_price = new_price;
    out.new_qty = new_qty;
    out.ts = new_order.ts;
    
    return true;
}

bool LimitBook::best_bid_ask(BookTop& out) const noexcept {
    out.best_bid = INVALID_PRICE;
    out.bid_qty = 0;
    out.best_ask = INVALID_PRICE;
    out.ask_qty = 0;
    out.ts = time_source_->now_ns();
    
    if (!bids_.empty()) {
        auto it = bids_.begin();
        out.best_bid = it->first;
        out.bid_qty = it->second.total_qty();
    }
    
    if (!asks_.empty()) {
        auto it = asks_.begin();
        out.best_ask = it->first;
        out.ask_qty = it->second.total_qty();
    }
    
    return !bids_.empty() || !asks_.empty();
}

Price LimitBook::best_price(Side side) const noexcept {
    if (side == Side::Buy) {
        return bids_.empty() ? INVALID_PRICE : bids_.begin()->first;
    } else {
        return asks_.empty() ? INVALID_PRICE : asks_.begin()->first;
    }
}

bool LimitBook::would_cross(const Order& order) const noexcept {
    if (order.is_market()) {
        return true; // Market orders always cross
    }
    
    Price best_opposite = best_price(opposite(order.side));
    if (best_opposite == INVALID_PRICE) {
        return false; // No opposite side
    }
    
    if (order.side == Side::Buy) {
        return order.price.ticks >= best_opposite.ticks;
    } else {
        return order.price.ticks <= best_opposite.ticks;
    }
}

void LimitBook::cleanup_empty_level(Side side, Price price) {
    if (side == Side::Buy) {
        auto it = bids_.find(price);
        if (it != bids_.end() && it->second.empty()) {
            bids_.erase(it);
        }
    } else {
        auto it = asks_.find(price);
        if (it != asks_.end() && it->second.empty()) {
            asks_.erase(it);
        }
    }
}

} // namespace lob
