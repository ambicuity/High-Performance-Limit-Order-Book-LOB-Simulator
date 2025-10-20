#pragma once

#include "MatchingEngine.h"
#include "Events.h"
#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <queue>

namespace lob {

// WebSocket feed configuration
struct WebSocketConfig {
    std::string host = "0.0.0.0";
    uint16_t port = 8080;
    size_t max_connections = 100;
    size_t buffer_size = 4096;
    
    WebSocketConfig() noexcept = default;
};

// Simplified WebSocket message for market data streaming
struct WebSocketMessage {
    std::string type;      // "trade", "booktop", "depth", etc.
    std::string data;      // JSON-formatted data
    uint64_t timestamp;
    
    WebSocketMessage() noexcept : timestamp(0) {}
    WebSocketMessage(const std::string& t, const std::string& d, uint64_t ts) 
        : type(t), data(d), timestamp(ts) {}
};

// WebSocket feed for live order book visualization
// Note: This is a simplified interface. Full implementation would require
// an actual WebSocket library like websocketpp or Boost.Beast
class WebSocketFeed {
public:
    using MessageCallback = std::function<void(const WebSocketMessage&)>;
    
    explicit WebSocketFeed(const WebSocketConfig& config = WebSocketConfig())
        : config_(config), running_(false) {}
    
    virtual ~WebSocketFeed() {
        stop();
    }
    
    // Start the WebSocket server
    virtual bool start() {
        if (running_.load()) {
            return false;
        }
        
        running_.store(true);
        worker_thread_ = std::thread(&WebSocketFeed::worker_loop, this);
        return true;
    }
    
    // Stop the WebSocket server
    virtual void stop() {
        if (!running_.load()) {
            return;
        }
        
        running_.store(false);
        cv_.notify_all();
        
        if (worker_thread_.joinable()) {
            worker_thread_.join();
        }
    }
    
    // Broadcast message to all connected clients
    void broadcast(const WebSocketMessage& msg) {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        message_queue_.push(msg);
        cv_.notify_one();
    }
    
    // Broadcast engine events as JSON
    void broadcast_event(const EngineEvent& event, const std::string& symbol = "") {
        WebSocketMessage msg;
        msg.timestamp = get_event_timestamp(event);
        
        std::visit([&](auto&& e) {
            using T = std::decay_t<decltype(e)>;
            if constexpr (std::is_same_v<T, TradeEvent>) {
                msg.type = "trade";
                msg.data = trade_to_json(e, symbol);
            } else if constexpr (std::is_same_v<T, BookTop>) {
                msg.type = "booktop";
                msg.data = booktop_to_json(e, symbol);
            } else if constexpr (std::is_same_v<T, AcceptEvent>) {
                msg.type = "accept";
                msg.data = accept_to_json(e, symbol);
            } else if constexpr (std::is_same_v<T, CancelEvent>) {
                msg.type = "cancel";
                msg.data = cancel_to_json(e, symbol);
            } else if constexpr (std::is_same_v<T, RejectEvent>) {
                msg.type = "reject";
                msg.data = reject_to_json(e, symbol);
            }
        }, event);
        
        broadcast(msg);
    }
    
    // Broadcast depth snapshot
    void broadcast_depth(const DepthSnapshot& depth, const std::string& symbol = "") {
        WebSocketMessage msg;
        msg.type = "depth";
        msg.timestamp = depth.ts;
        msg.data = depth_to_json(depth, symbol);
        broadcast(msg);
    }
    
    [[nodiscard]] bool is_running() const noexcept {
        return running_.load();
    }
    
    [[nodiscard]] const WebSocketConfig& config() const noexcept {
        return config_;
    }

protected:
    virtual void worker_loop() {
        // This would contain the actual WebSocket server logic
        // For now, it's a placeholder that processes the message queue
        while (running_.load()) {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            cv_.wait_for(lock, std::chrono::milliseconds(100), 
                        [this] { return !message_queue_.empty() || !running_.load(); });
            
            while (!message_queue_.empty()) {
                WebSocketMessage msg = message_queue_.front();
                message_queue_.pop();
                lock.unlock();
                
                // Actual WebSocket send would happen here
                // For demo purposes, we just process the message
                if (on_message_) {
                    on_message_(msg);
                }
                
                lock.lock();
            }
        }
    }
    
    void set_message_callback(MessageCallback cb) {
        on_message_ = std::move(cb);
    }

private:
    uint64_t get_event_timestamp(const EngineEvent& event) const {
        return std::visit([](auto&& e) -> uint64_t { return e.ts; }, event);
    }
    
    std::string trade_to_json(const TradeEvent& e, const std::string& symbol) const {
        return "{\"symbol\":\"" + symbol + 
               "\",\"taker_id\":" + std::to_string(e.taker_id) +
               ",\"maker_id\":" + std::to_string(e.maker_id) +
               ",\"price\":" + std::to_string(e.price.ticks) +
               ",\"qty\":" + std::to_string(e.qty) +
               ",\"ts\":" + std::to_string(e.ts) + "}";
    }
    
    std::string booktop_to_json(const BookTop& e, const std::string& symbol) const {
        return "{\"symbol\":\"" + symbol +
               "\",\"best_bid\":" + std::to_string(e.best_bid.ticks) +
               ",\"bid_qty\":" + std::to_string(e.bid_qty) +
               ",\"best_ask\":" + std::to_string(e.best_ask.ticks) +
               ",\"ask_qty\":" + std::to_string(e.ask_qty) +
               ",\"ts\":" + std::to_string(e.ts) + "}";
    }
    
    std::string accept_to_json(const AcceptEvent& e, const std::string& symbol) const {
        return "{\"symbol\":\"" + symbol +
               "\",\"order_id\":" + std::to_string(e.id) +
               ",\"ts\":" + std::to_string(e.ts) + "}";
    }
    
    std::string cancel_to_json(const CancelEvent& e, const std::string& symbol) const {
        return "{\"symbol\":\"" + symbol +
               "\",\"order_id\":" + std::to_string(e.id) +
               ",\"remaining\":" + std::to_string(e.remaining) +
               ",\"ts\":" + std::to_string(e.ts) + "}";
    }
    
    std::string reject_to_json(const RejectEvent& e, const std::string& symbol) const {
        return "{\"symbol\":\"" + symbol +
               "\",\"order_id\":" + std::to_string(e.id) +
               ",\"reason_code\":" + std::to_string(e.reason_code) +
               ",\"ts\":" + std::to_string(e.ts) + "}";
    }
    
    std::string depth_to_json(const DepthSnapshot& depth, const std::string& symbol) const {
        std::string json = "{\"symbol\":\"" + symbol + "\",\"bids\":[";
        for (size_t i = 0; i < depth.bids.size(); ++i) {
            if (i > 0) json += ",";
            const auto& level = depth.bids[i];
            json += "{\"price\":" + std::to_string(level.price.ticks) +
                   ",\"qty\":" + std::to_string(level.qty) +
                   ",\"orders\":" + std::to_string(level.order_count) + "}";
        }
        json += "],\"asks\":[";
        for (size_t i = 0; i < depth.asks.size(); ++i) {
            if (i > 0) json += ",";
            const auto& level = depth.asks[i];
            json += "{\"price\":" + std::to_string(level.price.ticks) +
                   ",\"qty\":" + std::to_string(level.qty) +
                   ",\"orders\":" + std::to_string(level.order_count) + "}";
        }
        json += "],\"ts\":" + std::to_string(depth.ts) + "}";
        return json;
    }
    
    WebSocketConfig config_;
    std::atomic<bool> running_;
    std::thread worker_thread_;
    std::queue<WebSocketMessage> message_queue_;
    std::mutex queue_mutex_;
    std::condition_variable cv_;
    MessageCallback on_message_;
};

} // namespace lob
