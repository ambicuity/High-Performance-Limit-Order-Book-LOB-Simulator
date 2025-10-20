#pragma once

#include <cstddef>
#include <vector>
#include <memory>
#include <new>

namespace lob {

// Memory pool for zero-allocation object management on hot paths
template<typename T>
class Pool {
public:
    explicit Pool(size_t capacity) {
        storage_.reserve(capacity);
        free_list_.reserve(capacity);
        
        // Pre-allocate all objects
        for (size_t i = 0; i < capacity; ++i) {
            storage_.emplace_back(std::make_unique<T>());
            free_list_.push_back(storage_.back().get());
        }
    }

    // Acquire an object from the pool
    [[nodiscard]] T* acquire() noexcept {
        if (free_list_.empty()) {
            return nullptr; // Pool exhausted
        }
        T* obj = free_list_.back();
        free_list_.pop_back();
        return obj;
    }

    // Release an object back to the pool
    void release(T* obj) noexcept {
        if (obj) {
            free_list_.push_back(obj);
        }
    }

    [[nodiscard]] size_t available() const noexcept {
        return free_list_.size();
    }

    [[nodiscard]] size_t capacity() const noexcept {
        return storage_.size();
    }

private:
    std::vector<std::unique_ptr<T>> storage_;
    std::vector<T*> free_list_;
};

} // namespace lob
