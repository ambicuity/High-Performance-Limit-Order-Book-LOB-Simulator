#include <gtest/gtest.h>
#include "lob/LimitBook.h"
#include "lob/MatchingEngine.h"
#include "lob/MultiSymbolEngine.h"
#include "lob/MarketDataReplay.h"
#include "lob/TimeSource.h"
#include <fstream>

using namespace lob;

// Test fixture for depth snapshot functionality
class DepthSnapshotTest : public ::testing::Test {
protected:
    void SetUp() override {
        time_source = std::make_shared<SimulatedTimeSource>(1000000);
        book = std::make_unique<LimitBook>(0.01, time_source);
    }

    std::shared_ptr<SimulatedTimeSource> time_source;
    std::unique_ptr<LimitBook> book;
};

TEST_F(DepthSnapshotTest, EmptyBookDepth) {
    DepthSnapshot depth;
    book->get_depth(depth, 5);
    
    EXPECT_TRUE(depth.bids.empty());
    EXPECT_TRUE(depth.asks.empty());
}

TEST_F(DepthSnapshotTest, SingleLevelDepth) {
    Order buy(1, Side::Buy, Price::from_double(100.0, 0.01), 50, time_source->now_ns());
    Order sell(2, Side::Sell, Price::from_double(100.5, 0.01), 60, time_source->now_ns());
    
    std::vector<TradeEvent> trades;
    book->add(buy, trades);
    book->add(sell, trades);
    
    DepthSnapshot depth;
    book->get_depth(depth, 5);
    
    ASSERT_EQ(depth.bids.size(), 1);
    ASSERT_EQ(depth.asks.size(), 1);
    
    EXPECT_EQ(depth.bids[0].price.to_double(0.01), 100.0);
    EXPECT_EQ(depth.bids[0].qty, 50);
    EXPECT_EQ(depth.bids[0].order_count, 1);
    
    EXPECT_EQ(depth.asks[0].price.to_double(0.01), 100.5);
    EXPECT_EQ(depth.asks[0].qty, 60);
    EXPECT_EQ(depth.asks[0].order_count, 1);
}

TEST_F(DepthSnapshotTest, MultiLevelDepth) {
    // Add multiple price levels
    std::vector<TradeEvent> trades;
    
    for (int i = 0; i < 5; i++) {
        Order buy(i + 1, Side::Buy, Price::from_double(100.0 - i * 0.05, 0.01), 
                 50 + i * 10, time_source->now_ns());
        book->add(buy, trades);
    }
    
    for (int i = 0; i < 5; i++) {
        Order sell(i + 11, Side::Sell, Price::from_double(100.05 + i * 0.05, 0.01), 
                  60 + i * 10, time_source->now_ns());
        book->add(sell, trades);
    }
    
    DepthSnapshot depth;
    book->get_depth(depth, 3);
    
    // Should have 3 levels on each side
    EXPECT_EQ(depth.bids.size(), 3);
    EXPECT_EQ(depth.asks.size(), 3);
    
    // Check ordering (bids descending, asks ascending)
    EXPECT_GT(depth.bids[0].price.ticks, depth.bids[1].price.ticks);
    EXPECT_LT(depth.asks[0].price.ticks, depth.asks[1].price.ticks);
}

TEST_F(DepthSnapshotTest, DepthWithMultipleOrdersPerLevel) {
    std::vector<TradeEvent> trades;
    
    // Add multiple orders at same price level
    Order buy1(1, Side::Buy, Price::from_double(100.0, 0.01), 30, time_source->now_ns());
    Order buy2(2, Side::Buy, Price::from_double(100.0, 0.01), 40, time_source->now_ns());
    Order buy3(3, Side::Buy, Price::from_double(100.0, 0.01), 50, time_source->now_ns());
    
    book->add(buy1, trades);
    book->add(buy2, trades);
    book->add(buy3, trades);
    
    DepthSnapshot depth;
    book->get_depth(depth, 5);
    
    ASSERT_EQ(depth.bids.size(), 1);
    EXPECT_EQ(depth.bids[0].qty, 120);  // 30 + 40 + 50
    EXPECT_EQ(depth.bids[0].order_count, 3);
}

// Test fixture for iceberg orders
class IcebergOrderTest : public ::testing::Test {
protected:
    void SetUp() override {
        time_source = std::make_shared<SimulatedTimeSource>(1000000);
        config = EngineConfig(100000, 10000, 0.01);
        engine = std::make_unique<MatchingEngine>(config, time_source);
    }

    std::shared_ptr<SimulatedTimeSource> time_source;
    EngineConfig config;
    std::unique_ptr<MatchingEngine> engine;
};

TEST_F(IcebergOrderTest, IcebergOrderCreation) {
    Order order(1, Side::Buy, Price::from_double(100.0, 0.01), 1000, time_source->now_ns());
    order.display_qty = 100;
    order.refresh_qty = 100;
    
    EXPECT_TRUE(order.is_iceberg());
    EXPECT_EQ(order.visible_qty(), 100);
    EXPECT_EQ(order.qty, 1000);
}

TEST_F(IcebergOrderTest, NonIcebergOrder) {
    Order order(1, Side::Buy, Price::from_double(100.0, 0.01), 1000, time_source->now_ns());
    
    EXPECT_FALSE(order.is_iceberg());
    EXPECT_EQ(order.visible_qty(), 1000);
}

// Test fixture for pegged orders
class PeggedOrderTest : public ::testing::Test {
protected:
    void SetUp() override {
        time_source = std::make_shared<SimulatedTimeSource>(1000000);
        config = EngineConfig(100000, 10000, 0.01);
        engine = std::make_unique<MatchingEngine>(config, time_source);
    }

    std::shared_ptr<SimulatedTimeSource> time_source;
    EngineConfig config;
    std::unique_ptr<MatchingEngine> engine;
};

TEST_F(PeggedOrderTest, MidPeggedOrder) {
    Order order(1, Side::Buy, Price::from_double(100.0, 0.01), 100, time_source->now_ns());
    order.peg_type = PegType::Mid;
    order.offset = -1;  // 1 tick below mid
    
    EXPECT_TRUE(order.is_pegged());
    EXPECT_EQ(order.peg_type, PegType::Mid);
}

TEST_F(PeggedOrderTest, BestBidPeggedOrder) {
    Order order(1, Side::Buy, Price::from_double(100.0, 0.01), 100, time_source->now_ns());
    order.peg_type = PegType::BestBid;
    order.offset = 0;
    
    EXPECT_TRUE(order.is_pegged());
    EXPECT_EQ(order.peg_type, PegType::BestBid);
}

TEST_F(PeggedOrderTest, NonPeggedOrder) {
    Order order(1, Side::Buy, Price::from_double(100.0, 0.01), 100, time_source->now_ns());
    
    EXPECT_FALSE(order.is_pegged());
    EXPECT_EQ(order.peg_type, PegType::None);
}

// Test fixture for multi-symbol engine
class MultiSymbolEngineTest : public ::testing::Test {
protected:
    void SetUp() override {
        time_source = std::make_shared<SimulatedTimeSource>(1000000);
        config = EngineConfig(100000, 10000, 0.01);
        multi_engine = std::make_unique<MultiSymbolEngine>(config, time_source);
    }

    std::shared_ptr<SimulatedTimeSource> time_source;
    EngineConfig config;
    std::unique_ptr<MultiSymbolEngine> multi_engine;
};

TEST_F(MultiSymbolEngineTest, AddSymbol) {
    EXPECT_TRUE(multi_engine->add_symbol("AAPL"));
    EXPECT_TRUE(multi_engine->add_symbol("GOOGL"));
    
    auto symbols = multi_engine->get_symbols();
    EXPECT_EQ(symbols.size(), 2);
}

TEST_F(MultiSymbolEngineTest, AddDuplicateSymbol) {
    EXPECT_TRUE(multi_engine->add_symbol("AAPL"));
    EXPECT_FALSE(multi_engine->add_symbol("AAPL"));  // Duplicate
}

TEST_F(MultiSymbolEngineTest, RemoveSymbol) {
    multi_engine->add_symbol("AAPL");
    EXPECT_TRUE(multi_engine->remove_symbol("AAPL"));
    EXPECT_FALSE(multi_engine->remove_symbol("AAPL"));  // Already removed
}

TEST_F(MultiSymbolEngineTest, SubmitToSymbol) {
    multi_engine->add_symbol("AAPL");
    
    Order order(1, Side::Buy, Price::from_double(150.0, 0.01), 100, time_source->now_ns());
    EXPECT_TRUE(multi_engine->submit("AAPL", order));
    
    // Verify order is in the book
    BookTop top;
    EXPECT_TRUE(multi_engine->best_bid_ask("AAPL", top));
    EXPECT_EQ(top.best_bid.to_double(0.01), 150.0);
    EXPECT_EQ(top.bid_qty, 100);
}

TEST_F(MultiSymbolEngineTest, SubmitToNonExistentSymbol) {
    Order order(1, Side::Buy, Price::from_double(150.0, 0.01), 100, time_source->now_ns());
    EXPECT_FALSE(multi_engine->submit("AAPL", order));  // Symbol not added
}

TEST_F(MultiSymbolEngineTest, IndependentOrderBooks) {
    multi_engine->add_symbol("AAPL");
    multi_engine->add_symbol("GOOGL");
    
    Order aapl_order(1, Side::Buy, Price::from_double(150.0, 0.01), 100, time_source->now_ns());
    Order googl_order(2, Side::Buy, Price::from_double(2800.0, 0.01), 50, time_source->now_ns());
    
    multi_engine->submit("AAPL", aapl_order);
    multi_engine->submit("GOOGL", googl_order);
    
    BookTop aapl_top, googl_top;
    multi_engine->best_bid_ask("AAPL", aapl_top);
    multi_engine->best_bid_ask("GOOGL", googl_top);
    
    EXPECT_EQ(aapl_top.best_bid.to_double(0.01), 150.0);
    EXPECT_EQ(googl_top.best_bid.to_double(0.01), 2800.0);
}

TEST_F(MultiSymbolEngineTest, GetDepthForSymbol) {
    multi_engine->add_symbol("AAPL");
    
    Order buy(1, Side::Buy, Price::from_double(150.0, 0.01), 100, time_source->now_ns());
    Order sell(2, Side::Sell, Price::from_double(150.5, 0.01), 100, time_source->now_ns());
    
    multi_engine->submit("AAPL", buy);
    multi_engine->submit("AAPL", sell);
    
    DepthSnapshot depth;
    multi_engine->get_depth("AAPL", depth, 5);
    
    EXPECT_EQ(depth.bids.size(), 1);
    EXPECT_EQ(depth.asks.size(), 1);
}

// Test fixture for market data replay
class MarketDataReplayTest : public ::testing::Test {
protected:
    void SetUp() override {
        time_source = std::make_shared<SimulatedTimeSource>(1000000);
        config = EngineConfig(100000, 10000, 0.01);
        engine = std::make_unique<MatchingEngine>(config, time_source);
        replay = std::make_unique<MarketDataReplay>(*engine);
        
        // Create temporary CSV file
        test_file = "/tmp/test_market_data.csv";
        std::ofstream file(test_file);
        file << "timestamp,action,order_id,side,price,qty,order_type\n";
        file << "1000000,ADD,1,BUY,100.00,50,LIMIT\n";
        file << "1001000,ADD,2,SELL,100.50,60,LIMIT\n";
        file << "1002000,CANCEL,1,BUY,0,0,LIMIT\n";
        file.close();
    }
    
    void TearDown() override {
        std::remove(test_file.c_str());
    }

    std::shared_ptr<SimulatedTimeSource> time_source;
    EngineConfig config;
    std::unique_ptr<MatchingEngine> engine;
    std::unique_ptr<MarketDataReplay> replay;
    std::string test_file;
};

TEST_F(MarketDataReplayTest, LoadCSV) {
    EXPECT_TRUE(replay->load_from_csv(test_file, 0.01));
    EXPECT_EQ(replay->message_count(), 3);
}

TEST_F(MarketDataReplayTest, LoadNonExistentFile) {
    EXPECT_FALSE(replay->load_from_csv("/tmp/nonexistent.csv", 0.01));
}

TEST_F(MarketDataReplayTest, ReplayAll) {
    replay->load_from_csv(test_file, 0.01);
    
    std::vector<EngineEvent> events;
    size_t processed = replay->replay_all(&events);
    
    EXPECT_EQ(processed, 3);
    EXPECT_GT(events.size(), 0);
}

TEST_F(MarketDataReplayTest, ReplayUntilTimestamp) {
    replay->load_from_csv(test_file, 0.01);
    
    // Replay only first two messages
    size_t processed = replay->replay_until(1001500);
    EXPECT_EQ(processed, 2);
    
    // Check that only first order is in book
    BookTop top;
    engine->best_bid_ask(top);
    EXPECT_EQ(top.best_ask.to_double(0.01), 100.50);
}

TEST_F(MarketDataReplayTest, ClearMessages) {
    replay->load_from_csv(test_file, 0.01);
    EXPECT_GT(replay->message_count(), 0);
    
    replay->clear();
    EXPECT_EQ(replay->message_count(), 0);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
