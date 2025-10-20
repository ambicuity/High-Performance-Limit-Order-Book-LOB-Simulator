#pragma once

#include "MatchingEngine.h"
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <optional>

namespace lob {

// Represents a recorded market data message
struct MarketDataMessage {
    uint64_t timestamp;    // Nanoseconds since epoch
    std::string action;    // "ADD", "CANCEL", "REPLACE", "TRADE"
    OrderId order_id;
    Side side;
    Price price;
    uint64_t qty;
    OrderType order_type;
    
    MarketDataMessage() noexcept
        : timestamp(0), order_id(0), side(Side::Buy), 
          price(), qty(0), order_type(OrderType::Limit) {}
};

// Market data replay engine for replaying historical order flow
class MarketDataReplay {
public:
    explicit MarketDataReplay(MatchingEngine& engine)
        : engine_(engine) {}
    
    // Load market data from CSV file
    // CSV format: timestamp,action,order_id,side,price,qty,order_type
    bool load_from_csv(const std::string& filename, double tick_size) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            return false;
        }
        
        messages_.clear();
        std::string line;
        
        // Skip header if present
        if (std::getline(file, line)) {
            if (line.find("timestamp") == std::string::npos) {
                // No header, process this line
                if (auto msg = parse_csv_line(line, tick_size)) {
                    messages_.push_back(*msg);
                }
            }
        }
        
        // Read data lines
        while (std::getline(file, line)) {
            if (auto msg = parse_csv_line(line, tick_size)) {
                messages_.push_back(*msg);
            }
        }
        
        return !messages_.empty();
    }
    
    // Replay all messages in order
    size_t replay_all(std::vector<EngineEvent>* out_events = nullptr) {
        size_t processed = 0;
        
        for (const auto& msg : messages_) {
            if (replay_message(msg)) {
                ++processed;
            }
            
            // Poll events if output vector provided
            if (out_events) {
                engine_.poll_events(*out_events);
            }
        }
        
        return processed;
    }
    
    // Replay messages up to specified timestamp
    size_t replay_until(uint64_t timestamp, std::vector<EngineEvent>* out_events = nullptr) {
        size_t processed = 0;
        
        for (const auto& msg : messages_) {
            if (msg.timestamp > timestamp) {
                break;
            }
            
            if (replay_message(msg)) {
                ++processed;
            }
            
            if (out_events) {
                engine_.poll_events(*out_events);
            }
        }
        
        return processed;
    }
    
    // Get total number of loaded messages
    [[nodiscard]] size_t message_count() const noexcept {
        return messages_.size();
    }
    
    // Clear all loaded messages
    void clear() noexcept {
        messages_.clear();
    }

private:
    bool replay_message(const MarketDataMessage& msg) {
        if (msg.action == "ADD" || msg.action == "SUBMIT") {
            Order order(msg.order_id, msg.side, msg.price, msg.qty, 
                       msg.timestamp, msg.order_type);
            return engine_.submit(order);
        } else if (msg.action == "CANCEL") {
            return engine_.cancel(msg.order_id);
        } else if (msg.action == "REPLACE") {
            return engine_.replace(msg.order_id, msg.price, msg.qty);
        }
        return false;
    }
    
    std::optional<MarketDataMessage> parse_csv_line(const std::string& line, double tick_size) {
        if (line.empty() || line[0] == '#') {
            return std::nullopt; // Skip empty lines and comments
        }
        
        std::istringstream ss(line);
        std::string field;
        MarketDataMessage msg;
        int field_idx = 0;
        
        while (std::getline(ss, field, ',')) {
            try {
                switch (field_idx) {
                    case 0: msg.timestamp = std::stoull(field); break;
                    case 1: msg.action = field; break;
                    case 2: msg.order_id = std::stoull(field); break;
                    case 3: 
                        msg.side = (field == "BUY" || field == "Buy" || field == "B") 
                                   ? Side::Buy : Side::Sell; 
                        break;
                    case 4: msg.price = Price::from_double(std::stod(field), tick_size); break;
                    case 5: msg.qty = std::stoull(field); break;
                    case 6: 
                        if (field == "MARKET" || field == "Market") {
                            msg.order_type = OrderType::Market;
                        } else if (field == "IOC") {
                            msg.order_type = OrderType::IOC;
                        } else if (field == "FOK") {
                            msg.order_type = OrderType::FOK;
                        } else {
                            msg.order_type = OrderType::Limit;
                        }
                        break;
                }
                ++field_idx;
            } catch (...) {
                return std::nullopt; // Parse error
            }
        }
        
        return field_idx >= 6 ? std::optional<MarketDataMessage>(msg) : std::nullopt;
    }
    
    MatchingEngine& engine_;
    std::vector<MarketDataMessage> messages_;
};

} // namespace lob
