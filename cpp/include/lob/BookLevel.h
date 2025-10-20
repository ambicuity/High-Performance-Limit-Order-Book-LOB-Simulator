#pragma once

#include "Order.h"
#include "OrderId.h"
#include <list>
#include <cstdint>

namespace lob {

// Represents a single order in the book with intrusive list support
struct BookOrder {
    Order order;
    uint64_t remaining_qty;
    
    BookOrder() noexcept : order(), remaining_qty(0) {}
    explicit BookOrder(const Order& o) noexcept 
        : order(o), remaining_qty(o.qty) {}
};

// A price level maintaining FIFO queue of orders
class BookLevel {
public:
    BookLevel() noexcept = default;

    // Add order to back of queue (FIFO)
    void add_order(const BookOrder& order) {
        orders_.push_back(order);
        total_qty_ += order.remaining_qty;
    }

    // Get first order in queue
    [[nodiscard]] BookOrder* front() noexcept {
        return orders_.empty() ? nullptr : &orders_.front();
    }

    // Remove first order from queue
    void pop_front() noexcept {
        if (!orders_.empty()) {
            total_qty_ -= orders_.front().remaining_qty;
            orders_.pop_front();
        }
    }

    // Find and remove order by ID
    [[nodiscard]] bool remove_order(OrderId id, uint64_t& removed_qty) noexcept {
        for (auto it = orders_.begin(); it != orders_.end(); ++it) {
            if (it->order.id == id) {
                removed_qty = it->remaining_qty;
                total_qty_ -= removed_qty;
                orders_.erase(it);
                return true;
            }
        }
        return false;
    }

    // Find order by ID
    [[nodiscard]] BookOrder* find_order(OrderId id) noexcept {
        for (auto& order : orders_) {
            if (order.order.id == id) {
                return &order;
            }
        }
        return nullptr;
    }

    // Update quantity of first order
    void update_front_qty(uint64_t new_qty) noexcept {
        if (!orders_.empty()) {
            uint64_t old_qty = orders_.front().remaining_qty;
            orders_.front().remaining_qty = new_qty;
            total_qty_ = total_qty_ - old_qty + new_qty;
        }
    }

    [[nodiscard]] bool empty() const noexcept {
        return orders_.empty();
    }

    [[nodiscard]] size_t size() const noexcept {
        return orders_.size();
    }

    [[nodiscard]] uint64_t total_qty() const noexcept {
        return total_qty_;
    }

private:
    std::list<BookOrder> orders_;
    uint64_t total_qty_ = 0;
};

} // namespace lob
