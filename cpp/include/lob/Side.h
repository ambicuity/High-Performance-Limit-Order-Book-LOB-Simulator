#pragma once

#include <cstdint>

namespace lob {

enum class Side : uint8_t {
    Buy = 0,
    Sell = 1
};

constexpr Side opposite(Side s) noexcept {
    return s == Side::Buy ? Side::Sell : Side::Buy;
}

} // namespace lob
