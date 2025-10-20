#pragma once

#include <atomic>
#include <cstddef>
#include <vector>
#include <cstring>

namespace lob {

// Lock-free SPSC (Single Producer Single Consumer) ring buffer
template<typename T>
class RingBuffer {
public:
    explicit RingBuffer(size_t capacity) 
        : capacity_(next_power_of_two(capacity))
        , mask_(capacity_ - 1)
        , buffer_(capacity_)
        , head_(0)
        , tail_(0) {
    }

    // Push an item (producer side)
    [[nodiscard]] bool push(const T& item) noexcept {
        size_t current_tail = tail_.load(std::memory_order_relaxed);
        size_t next_tail = (current_tail + 1) & mask_;
        
        if (next_tail == head_.load(std::memory_order_acquire)) {
            return false; // Buffer full
        }
        
        buffer_[current_tail] = item;
        tail_.store(next_tail, std::memory_order_release);
        return true;
    }

    // Pop an item (consumer side)
    [[nodiscard]] bool pop(T& item) noexcept {
        size_t current_head = head_.load(std::memory_order_relaxed);
        
        if (current_head == tail_.load(std::memory_order_acquire)) {
            return false; // Buffer empty
        }
        
        item = buffer_[current_head];
        head_.store((current_head + 1) & mask_, std::memory_order_release);
        return true;
    }

    [[nodiscard]] bool empty() const noexcept {
        return head_.load(std::memory_order_acquire) == 
               tail_.load(std::memory_order_acquire);
    }

    [[nodiscard]] size_t size() const noexcept {
        size_t h = head_.load(std::memory_order_acquire);
        size_t t = tail_.load(std::memory_order_acquire);
        return (t - h) & mask_;
    }

    [[nodiscard]] size_t capacity() const noexcept {
        return capacity_;
    }

private:
    static size_t next_power_of_two(size_t n) noexcept {
        if (n == 0) return 1;
        n--;
        n |= n >> 1;
        n |= n >> 2;
        n |= n >> 4;
        n |= n >> 8;
        n |= n >> 16;
        n |= n >> 32;
        return n + 1;
    }

    const size_t capacity_;
    const size_t mask_;
    std::vector<T> buffer_;
    alignas(64) std::atomic<size_t> head_;
    alignas(64) std::atomic<size_t> tail_;
};

} // namespace lob
