#include <gtest/gtest.h>
#include "lob/MatchingEngine.h"
#include "lob/TimeSource.h"

using namespace lob;

class MatchingEngineTest : public ::testing::Test {
protected:
    void SetUp() override {
        EngineConfig config;
        config.max_orders = 1000;
        config.ring_size = 1000;
        config.tick_size = 0.01;
        
        time_source = std::make_shared<SimulatedTimeSource>(1000000);
        engine = std::make_unique<MatchingEngine>(config, time_source);
    }

    std::shared_ptr<SimulatedTimeSource> time_source;
    std::unique_ptr<MatchingEngine> engine;
};

TEST_F(MatchingEngineTest, SubmitOrderGeneratesAcceptEvent) {
    Order order(1, Side::Buy, Price::from_double(100.0, 0.01), 10, 1000000, OrderType::Limit);
    EXPECT_TRUE(engine->submit(order));
    
    std::vector<EngineEvent> events;
    EXPECT_TRUE(engine->poll_events(events));
    
    // Should have accept event and book update
    EXPECT_GE(events.size(), 2);
    EXPECT_TRUE(std::holds_alternative<AcceptEvent>(events[0]));
}

TEST_F(MatchingEngineTest, MatchGeneratesTradeEvent) {
    // Submit sell order
    Order sell(1, Side::Sell, Price::from_double(100.0, 0.01), 10, 1000000, OrderType::Limit);
    EXPECT_TRUE(engine->submit(sell));
    
    std::vector<EngineEvent> events;
    engine->poll_events(events);
    events.clear();
    
    // Submit matching buy order
    Order buy(2, Side::Buy, Price::from_double(100.0, 0.01), 10, 1000001, OrderType::Limit);
    EXPECT_TRUE(engine->submit(buy));
    
    EXPECT_TRUE(engine->poll_events(events));
    
    // Check for trade event
    bool found_trade = false;
    for (const auto& event : events) {
        if (std::holds_alternative<TradeEvent>(event)) {
            const auto& trade = std::get<TradeEvent>(event);
            EXPECT_EQ(trade.qty, 10);
            found_trade = true;
        }
    }
    EXPECT_TRUE(found_trade);
}

TEST_F(MatchingEngineTest, CancelGeneratesCancelEvent) {
    Order order(1, Side::Buy, Price::from_double(100.0, 0.01), 10, 1000000, OrderType::Limit);
    EXPECT_TRUE(engine->submit(order));
    
    std::vector<EngineEvent> events;
    engine->poll_events(events);
    events.clear();
    
    EXPECT_TRUE(engine->cancel(1));
    EXPECT_TRUE(engine->poll_events(events));
    
    bool found_cancel = false;
    for (const auto& event : events) {
        if (std::holds_alternative<CancelEvent>(event)) {
            const auto& cancel = std::get<CancelEvent>(event);
            EXPECT_EQ(cancel.id, 1);
            EXPECT_EQ(cancel.remaining, 10);
            found_cancel = true;
        }
    }
    EXPECT_TRUE(found_cancel);
}

TEST_F(MatchingEngineTest, ReplaceGeneratesReplaceEvent) {
    Order order(1, Side::Buy, Price::from_double(100.0, 0.01), 10, 1000000, OrderType::Limit);
    EXPECT_TRUE(engine->submit(order));
    
    std::vector<EngineEvent> events;
    engine->poll_events(events);
    events.clear();
    
    Price new_price = Price::from_double(101.0, 0.01);
    EXPECT_TRUE(engine->replace(1, new_price, 15));
    EXPECT_TRUE(engine->poll_events(events));
    
    bool found_replace = false;
    for (const auto& event : events) {
        if (std::holds_alternative<ReplaceEvent>(event)) {
            const auto& replace = std::get<ReplaceEvent>(event);
            EXPECT_EQ(replace.id, 1);
            EXPECT_EQ(replace.new_price.to_double(0.01), 101.0);
            EXPECT_EQ(replace.new_qty, 15);
            found_replace = true;
        }
    }
    EXPECT_TRUE(found_replace);
}

TEST_F(MatchingEngineTest, BestBidAsk) {
    Order buy(1, Side::Buy, Price::from_double(100.0, 0.01), 10, 1000000, OrderType::Limit);
    Order sell(2, Side::Sell, Price::from_double(101.0, 0.01), 15, 1000001, OrderType::Limit);
    
    EXPECT_TRUE(engine->submit(buy));
    EXPECT_TRUE(engine->submit(sell));
    
    BookTop top;
    EXPECT_TRUE(engine->best_bid_ask(top));
    EXPECT_EQ(top.best_bid.to_double(0.01), 100.0);
    EXPECT_EQ(top.bid_qty, 10);
    EXPECT_EQ(top.best_ask.to_double(0.01), 101.0);
    EXPECT_EQ(top.ask_qty, 15);
}

TEST_F(MatchingEngineTest, TimeSourceProgression) {
    uint64_t start_time = engine->now();
    time_source->advance(1000000); // Advance 1ms
    uint64_t end_time = engine->now();
    
    EXPECT_EQ(end_time - start_time, 1000000);
}

TEST_F(MatchingEngineTest, MultipleOrdersAndTrades) {
    // Submit multiple orders
    for (int i = 0; i < 10; i++) {
        double price = 100.0 + i * 0.01;
        Order buy(i * 2 + 1, Side::Buy, Price::from_double(price, 0.01), 10, 1000000 + i, OrderType::Limit);
        Order sell(i * 2 + 2, Side::Sell, Price::from_double(price + 1.0, 0.01), 10, 1000000 + i + 100, OrderType::Limit);
        
        EXPECT_TRUE(engine->submit(buy));
        EXPECT_TRUE(engine->submit(sell));
    }
    
    std::vector<EngineEvent> events;
    engine->poll_events(events);
    
    // Should have accept events and book updates
    EXPECT_GT(events.size(), 20);
}

TEST_F(MatchingEngineTest, RejectDuplicateOrderId) {
    Order order1(1, Side::Buy, Price::from_double(100.0, 0.01), 10, 1000000, OrderType::Limit);
    Order order2(1, Side::Sell, Price::from_double(101.0, 0.01), 10, 1000001, OrderType::Limit);
    
    EXPECT_TRUE(engine->submit(order1));
    EXPECT_FALSE(engine->submit(order2)); // Same ID, should reject
    
    std::vector<EngineEvent> events;
    engine->poll_events(events);
    
    // Check for reject event
    bool found_reject = false;
    for (const auto& event : events) {
        if (std::holds_alternative<RejectEvent>(event)) {
            found_reject = true;
        }
    }
    EXPECT_TRUE(found_reject);
}

TEST_F(MatchingEngineTest, PartialFillSequence) {
    // Add sell order
    Order sell(1, Side::Sell, Price::from_double(100.0, 0.01), 100, 1000000, OrderType::Limit);
    EXPECT_TRUE(engine->submit(sell));
    
    std::vector<EngineEvent> events;
    engine->poll_events(events);
    events.clear();
    
    // Partially fill with multiple small buys
    for (int i = 0; i < 5; i++) {
        Order buy(i + 10, Side::Buy, Price::from_double(100.0, 0.01), 10, 1000001 + i, OrderType::Limit);
        EXPECT_TRUE(engine->submit(buy));
    }
    
    EXPECT_TRUE(engine->poll_events(events));
    
    // Count trade events
    int trade_count = 0;
    for (const auto& event : events) {
        if (std::holds_alternative<TradeEvent>(event)) {
            trade_count++;
        }
    }
    EXPECT_EQ(trade_count, 5);
}

TEST_F(MatchingEngineTest, Determinism) {
    // Same sequence of orders should produce same results
    std::vector<Order> orders;
    for (int i = 0; i < 10; i++) {
        orders.push_back(Order(i + 1, Side::Buy, Price::from_double(100.0 - i * 0.1, 0.01), 
                              10, 1000000 + i * 1000, OrderType::Limit));
    }
    for (int i = 0; i < 10; i++) {
        orders.push_back(Order(i + 11, Side::Sell, Price::from_double(100.5 + i * 0.1, 0.01), 
                              10, 1000000 + (i + 10) * 1000, OrderType::Limit));
    }
    
    for (const auto& order : orders) {
        EXPECT_TRUE(engine->submit(order));
    }
    
    std::vector<EngineEvent> events;
    engine->poll_events(events);
    
    // Book state should be predictable
    BookTop top;
    EXPECT_TRUE(engine->best_bid_ask(top));
    EXPECT_EQ(top.best_bid.to_double(0.01), 100.0);
    EXPECT_EQ(top.best_ask.to_double(0.01), 100.5);
}
