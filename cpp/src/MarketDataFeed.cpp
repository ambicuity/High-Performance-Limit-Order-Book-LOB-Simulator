#include "lob/MarketDataFeed.h"
#include <algorithm>

namespace lob {

bool MarketDataFeed::parse_csv_line(const std::string& line, 
                                     std::vector<std::string>& out_fields) {
    out_fields.clear();
    std::stringstream ss(line);
    std::string field;
    
    while (std::getline(ss, field, ',')) {
        // Trim whitespace
        field.erase(0, field.find_first_not_of(" \t\r\n"));
        field.erase(field.find_last_not_of(" \t\r\n") + 1);
        out_fields.push_back(field);
    }
    
    return !out_fields.empty();
}

bool MarketDataFeed::load_orders(const std::string& filename, 
                                  std::vector<MDOrder>& out_orders) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        return false;
    }
    
    out_orders.clear();
    std::string line;
    bool first_line = true;
    
    while (std::getline(file, line)) {
        // Skip header
        if (first_line) {
            first_line = false;
            continue;
        }
        
        if (line.empty()) continue;
        
        std::vector<std::string> fields;
        if (!parse_csv_line(line, fields)) continue;
        
        // Expected format: ts_ns,order_id,side,px,qty,type[,new_px,new_qty]
        if (fields.size() < 6) continue;
        
        MDOrder order;
        try {
            order.ts_ns = std::stoull(fields[0]);
            order.order_id = std::stoull(fields[1]);
            order.side = (fields[2] == "buy" || fields[2] == "Buy" || fields[2] == "BUY") 
                         ? Side::Buy : Side::Sell;
            order.price = std::stod(fields[3]);
            order.qty = std::stoull(fields[4]);
            order.type = fields[5];
            
            if (fields.size() >= 8) {
                order.new_price = std::stod(fields[6]);
                order.new_qty = std::stoull(fields[7]);
            } else {
                order.new_price = 0.0;
                order.new_qty = 0;
            }
            
            out_orders.push_back(order);
        } catch (...) {
            // Skip malformed lines
            continue;
        }
    }
    
    return true;
}

bool MarketDataFeed::load_quotes(const std::string& filename, 
                                  std::vector<MDQuote>& out_quotes) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        return false;
    }
    
    out_quotes.clear();
    std::string line;
    bool first_line = true;
    
    while (std::getline(file, line)) {
        // Skip header
        if (first_line) {
            first_line = false;
            continue;
        }
        
        if (line.empty()) continue;
        
        std::vector<std::string> fields;
        if (!parse_csv_line(line, fields)) continue;
        
        // Expected format: ts_ns,bid,ask,bid_qty,ask_qty
        if (fields.size() < 5) continue;
        
        MDQuote quote;
        try {
            quote.ts_ns = std::stoull(fields[0]);
            quote.bid = std::stod(fields[1]);
            quote.ask = std::stod(fields[2]);
            quote.bid_qty = std::stoull(fields[3]);
            quote.ask_qty = std::stoull(fields[4]);
            
            out_quotes.push_back(quote);
        } catch (...) {
            continue;
        }
    }
    
    return true;
}

bool MarketDataFeed::load_trades(const std::string& filename, 
                                  std::vector<MDTrade>& out_trades) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        return false;
    }
    
    out_trades.clear();
    std::string line;
    bool first_line = true;
    
    while (std::getline(file, line)) {
        // Skip header
        if (first_line) {
            first_line = false;
            continue;
        }
        
        if (line.empty()) continue;
        
        std::vector<std::string> fields;
        if (!parse_csv_line(line, fields)) continue;
        
        // Expected format: ts_ns,price,qty
        if (fields.size() < 3) continue;
        
        MDTrade trade;
        try {
            trade.ts_ns = std::stoull(fields[0]);
            trade.price = std::stod(fields[1]);
            trade.qty = std::stoull(fields[2]);
            
            out_trades.push_back(trade);
        } catch (...) {
            continue;
        }
    }
    
    return true;
}

Order MarketDataFeed::to_order(const MDOrder& md_order, double tick_size) {
    Order order;
    order.id = md_order.order_id;
    order.side = md_order.side;
    order.price = Price::from_double(md_order.price, tick_size);
    order.qty = md_order.qty;
    order.ts = md_order.ts_ns;
    order.type = parse_order_type(md_order.type);
    return order;
}

OrderType MarketDataFeed::parse_order_type(const std::string& type_str) {
    std::string lower_type = type_str;
    std::transform(lower_type.begin(), lower_type.end(), lower_type.begin(), ::tolower);
    
    if (lower_type == "limit") return OrderType::Limit;
    if (lower_type == "market") return OrderType::Market;
    if (lower_type == "ioc") return OrderType::IOC;
    if (lower_type == "fok") return OrderType::FOK;
    
    return OrderType::Limit; // Default
}

} // namespace lob
