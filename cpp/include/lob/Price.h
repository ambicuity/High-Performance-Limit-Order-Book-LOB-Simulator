#pragma once

#include <cstdint>
#include <compare>
#include <functional>

namespace lob {

// Price stored as integer ticks to avoid floating-point comparison issues
struct Price {
    int64_t ticks;

    constexpr Price() noexcept : ticks(0) {}
    constexpr explicit Price(int64_t t) noexcept : ticks(t) {}

    // Convert from double using tick size
    static Price from_double(double price, double tick_size) noexcept {
        return Price(static_cast<int64_t>(price / tick_size + 0.5));
    }

    // Convert to double using tick size
    [[nodiscard]] double to_double(double tick_size) const noexcept {
        return ticks * tick_size;
    }

    constexpr auto operator<=>(const Price&) const noexcept = default;
    constexpr bool operator==(const Price&) const noexcept = default;
};

constexpr Price INVALID_PRICE{-1};

} // namespace lob

// Hash support for Price
namespace std {
    template<>
    struct hash<lob::Price> {
        size_t operator()(const lob::Price& p) const noexcept {
            return std::hash<int64_t>{}(p.ticks);
        }
    };
}
