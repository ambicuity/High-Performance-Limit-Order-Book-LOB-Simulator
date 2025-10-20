#pragma once

#include "Order.h"
#include "Events.h"
#include <string>
#include <vector>
#include <fstream>
#include <sstream>

namespace lob {

// Market data record types
enum class MDRecordType {
    Order,      // Order submission/cancel/replace
    Quote,      // Bid/ask quote update
    Trade       // Executed trade
};

struct MDOrder {
    uint64_t ts_ns;
    OrderId order_id;
    Side side;
    double price;
    uint64_t qty;
    std::string type;  // "limit", "market", "ioc", "fok", "cancel", "replace"
    double new_price;  // For replace orders
    uint64_t new_qty;  // For replace orders
};

struct MDQuote {
    uint64_t ts_ns;
    double bid;
    double ask;
    uint64_t bid_qty;
    uint64_t ask_qty;
};

struct MDTrade {
    uint64_t ts_ns;
    double price;
    uint64_t qty;
};

class MarketDataFeed {
public:
    MarketDataFeed() = default;

    // Load orders from CSV file
    [[nodiscard]] bool load_orders(const std::string& filename, 
                                    std::vector<MDOrder>& out_orders);

    // Load quotes from CSV file
    [[nodiscard]] bool load_quotes(const std::string& filename, 
                                    std::vector<MDQuote>& out_quotes);

    // Load trades from CSV file
    [[nodiscard]] bool load_trades(const std::string& filename, 
                                    std::vector<MDTrade>& out_trades);

    // Convert MDOrder to Order
    [[nodiscard]] static Order to_order(const MDOrder& md_order, double tick_size);

    // Parse order type string
    [[nodiscard]] static OrderType parse_order_type(const std::string& type_str);

private:
    [[nodiscard]] static bool parse_csv_line(const std::string& line, 
                                              std::vector<std::string>& out_fields);
};

} // namespace lob
