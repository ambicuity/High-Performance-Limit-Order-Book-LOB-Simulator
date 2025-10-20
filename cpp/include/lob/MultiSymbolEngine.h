#pragma once

#include "MatchingEngine.h"
#include <unordered_map>
#include <string>
#include <memory>
#include <shared_mutex>

namespace lob {

using SymbolId = std::string;

// Multi-symbol matching engine with per-symbol order books
class MultiSymbolEngine {
public:
    explicit MultiSymbolEngine(const EngineConfig& default_config,
                              std::shared_ptr<TimeSource> time_source = nullptr)
        : default_config_(default_config)
        , time_source_(time_source ? time_source : std::make_shared<SimulatedTimeSource>()) {}

    // Add a new symbol with optional custom config
    bool add_symbol(const SymbolId& symbol, const EngineConfig* custom_config = nullptr) {
        std::unique_lock lock(mutex_);
        
        if (engines_.find(symbol) != engines_.end()) {
            return false; // Symbol already exists
        }
        
        const EngineConfig& config = custom_config ? *custom_config : default_config_;
        engines_[symbol] = std::make_unique<MatchingEngine>(config, time_source_);
        return true;
    }
    
    // Remove a symbol and its order book
    bool remove_symbol(const SymbolId& symbol) {
        std::unique_lock lock(mutex_);
        return engines_.erase(symbol) > 0;
    }
    
    // Submit order to specific symbol
    [[nodiscard]] bool submit(const SymbolId& symbol, const Order& order) {
        std::shared_lock lock(mutex_);
        auto it = engines_.find(symbol);
        if (it == engines_.end()) {
            return false; // Symbol not found
        }
        return it->second->submit(order);
    }
    
    // Cancel order from specific symbol
    [[nodiscard]] bool cancel(const SymbolId& symbol, OrderId id) {
        std::shared_lock lock(mutex_);
        auto it = engines_.find(symbol);
        if (it == engines_.end()) {
            return false;
        }
        return it->second->cancel(id);
    }
    
    // Replace order in specific symbol
    [[nodiscard]] bool replace(const SymbolId& symbol, OrderId id, 
                               Price new_price, uint64_t new_qty) {
        std::shared_lock lock(mutex_);
        auto it = engines_.find(symbol);
        if (it == engines_.end()) {
            return false;
        }
        return it->second->replace(id, new_price, new_qty);
    }
    
    // Get best bid/ask for specific symbol
    [[nodiscard]] bool best_bid_ask(const SymbolId& symbol, BookTop& out) const {
        std::shared_lock lock(mutex_);
        auto it = engines_.find(symbol);
        if (it == engines_.end()) {
            return false;
        }
        return it->second->best_bid_ask(out);
    }
    
    // Get depth snapshot for specific symbol
    void get_depth(const SymbolId& symbol, DepthSnapshot& out, size_t max_levels = 10) const {
        std::shared_lock lock(mutex_);
        auto it = engines_.find(symbol);
        if (it != engines_.end()) {
            it->second->get_depth(out, max_levels);
        }
    }
    
    // Poll events from specific symbol
    [[nodiscard]] bool poll_events(const SymbolId& symbol, std::vector<EngineEvent>& out_events) {
        std::shared_lock lock(mutex_);
        auto it = engines_.find(symbol);
        if (it == engines_.end()) {
            return false;
        }
        return it->second->poll_events(out_events);
    }
    
    // Get list of all symbols
    std::vector<SymbolId> get_symbols() const {
        std::shared_lock lock(mutex_);
        std::vector<SymbolId> symbols;
        symbols.reserve(engines_.size());
        for (const auto& [symbol, _] : engines_) {
            symbols.push_back(symbol);
        }
        return symbols;
    }
    
    // Get access to specific engine (for advanced operations)
    MatchingEngine* get_engine(const SymbolId& symbol) {
        std::shared_lock lock(mutex_);
        auto it = engines_.find(symbol);
        return it != engines_.end() ? it->second.get() : nullptr;
    }

private:
    EngineConfig default_config_;
    std::shared_ptr<TimeSource> time_source_;
    std::unordered_map<SymbolId, std::unique_ptr<MatchingEngine>> engines_;
    mutable std::shared_mutex mutex_;
};

} // namespace lob
