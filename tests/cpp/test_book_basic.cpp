#include <gtest/gtest.h>
#include "lob/LimitBook.h"
#include "lob/TimeSource.h"

using namespace lob;

class LimitBookTest : public ::testing::Test {
protected:
    void SetUp() override {
        time_source = std::make_shared<SimulatedTimeSource>(1000000);
        book = std::make_unique<LimitBook>(0.01, time_source);
    }

    std::shared_ptr<SimulatedTimeSource> time_source;
    std::unique_ptr<LimitBook> book;
};

TEST_F(LimitBookTest, EmptyBookBestBidAsk) {
    BookTop top;
    bool has_data = book->best_bid_ask(top);
    
    EXPECT_FALSE(has_data);
}

TEST_F(LimitBookTest, AddSingleBuyOrder) {
    Order order(1, Side::Buy, Price::from_double(100.0, 0.01), 10, 1000000, OrderType::Limit);
    std::vector<TradeEvent> trades;
    BookTop top;
    
    EXPECT_TRUE(book->add(order, trades, &top));
    EXPECT_TRUE(trades.empty());
    EXPECT_EQ(book->total_orders(), 1);
    EXPECT_EQ(top.best_bid.to_double(0.01), 100.0);
    EXPECT_EQ(top.bid_qty, 10);
}

TEST_F(LimitBookTest, AddSingleSellOrder) {
    Order order(2, Side::Sell, Price::from_double(101.0, 0.01), 15, 1000000, OrderType::Limit);
    std::vector<TradeEvent> trades;
    BookTop top;
    
    EXPECT_TRUE(book->add(order, trades, &top));
    EXPECT_TRUE(trades.empty());
    EXPECT_EQ(book->total_orders(), 1);
    EXPECT_EQ(top.best_ask.to_double(0.01), 101.0);
    EXPECT_EQ(top.ask_qty, 15);
}

TEST_F(LimitBookTest, CrossingOrdersMatchFully) {
    // Add sell order first
    Order sell_order(1, Side::Sell, Price::from_double(100.0, 0.01), 10, 1000000, OrderType::Limit);
    std::vector<TradeEvent> trades;
    EXPECT_TRUE(book->add(sell_order, trades));
    EXPECT_TRUE(trades.empty());
    
    // Add crossing buy order
    Order buy_order(2, Side::Buy, Price::from_double(100.0, 0.01), 10, 1000001, OrderType::Limit);
    trades.clear();
    EXPECT_TRUE(book->add(buy_order, trades));
    
    EXPECT_EQ(trades.size(), 1);
    EXPECT_EQ(trades[0].taker_id, 2);
    EXPECT_EQ(trades[0].maker_id, 1);
    EXPECT_EQ(trades[0].qty, 10);
    EXPECT_EQ(trades[0].price.to_double(0.01), 100.0);
    EXPECT_EQ(book->total_orders(), 0);
}

TEST_F(LimitBookTest, PartialFill) {
    // Add sell order
    Order sell_order(1, Side::Sell, Price::from_double(100.0, 0.01), 10, 1000000, OrderType::Limit);
    std::vector<TradeEvent> trades;
    EXPECT_TRUE(book->add(sell_order, trades));
    
    // Add smaller buy order
    Order buy_order(2, Side::Buy, Price::from_double(100.0, 0.01), 5, 1000001, OrderType::Limit);
    trades.clear();
    EXPECT_TRUE(book->add(buy_order, trades));
    
    EXPECT_EQ(trades.size(), 1);
    EXPECT_EQ(trades[0].qty, 5);
    EXPECT_EQ(book->total_orders(), 1); // Sell order partially filled
}

TEST_F(LimitBookTest, CancelOrder) {
    Order order(1, Side::Buy, Price::from_double(100.0, 0.01), 10, 1000000, OrderType::Limit);
    std::vector<TradeEvent> trades;
    EXPECT_TRUE(book->add(order, trades));
    EXPECT_EQ(book->total_orders(), 1);
    
    CancelEvent cancel_event;
    EXPECT_TRUE(book->cancel(1, cancel_event));
    EXPECT_EQ(cancel_event.id, 1);
    EXPECT_EQ(cancel_event.remaining, 10);
    EXPECT_EQ(book->total_orders(), 0);
}

TEST_F(LimitBookTest, CancelNonExistentOrder) {
    CancelEvent cancel_event;
    EXPECT_FALSE(book->cancel(999, cancel_event));
}

TEST_F(LimitBookTest, PriceTimePriority) {
    // Add two sell orders at same price
    Order sell1(1, Side::Sell, Price::from_double(100.0, 0.01), 10, 1000000, OrderType::Limit);
    Order sell2(2, Side::Sell, Price::from_double(100.0, 0.01), 10, 1000001, OrderType::Limit);
    
    std::vector<TradeEvent> trades;
    EXPECT_TRUE(book->add(sell1, trades));
    EXPECT_TRUE(book->add(sell2, trades));
    
    // Add buy order that matches one
    Order buy(3, Side::Buy, Price::from_double(100.0, 0.01), 10, 1000002, OrderType::Limit);
    trades.clear();
    EXPECT_TRUE(book->add(buy, trades));
    
    // Should match with first sell order (time priority)
    EXPECT_EQ(trades.size(), 1);
    EXPECT_EQ(trades[0].maker_id, 1); // First order has priority
    EXPECT_EQ(book->total_orders(), 1); // Second sell order remains
}

TEST_F(LimitBookTest, MarketOrderBuy) {
    // Add sell orders
    Order sell1(1, Side::Sell, Price::from_double(100.0, 0.01), 5, 1000000, OrderType::Limit);
    Order sell2(2, Side::Sell, Price::from_double(101.0, 0.01), 5, 1000001, OrderType::Limit);
    
    std::vector<TradeEvent> trades;
    EXPECT_TRUE(book->add(sell1, trades));
    EXPECT_TRUE(book->add(sell2, trades));
    
    // Market buy should sweep
    Order market_buy(3, Side::Buy, Price(0), 8, 1000002, OrderType::Market);
    trades.clear();
    EXPECT_TRUE(book->add(market_buy, trades));
    
    EXPECT_EQ(trades.size(), 2);
    EXPECT_EQ(trades[0].qty, 5); // First at 100.0
    EXPECT_EQ(trades[1].qty, 3); // Partial at 101.0
}

TEST_F(LimitBookTest, IOCOrderPartialFill) {
    // Add sell order
    Order sell(1, Side::Sell, Price::from_double(100.0, 0.01), 5, 1000000, OrderType::Limit);
    std::vector<TradeEvent> trades;
    EXPECT_TRUE(book->add(sell, trades));
    
    // IOC buy for more than available
    Order ioc_buy(2, Side::Buy, Price::from_double(100.0, 0.01), 10, 1000001, OrderType::IOC);
    trades.clear();
    EXPECT_TRUE(book->add(ioc_buy, trades));
    
    EXPECT_EQ(trades.size(), 1);
    EXPECT_EQ(trades[0].qty, 5); // Only 5 filled
    EXPECT_EQ(book->total_orders(), 0); // IOC doesn't rest, sell fully filled
}

TEST_F(LimitBookTest, FOKOrderFullFill) {
    // Add sell order with enough quantity
    Order sell(1, Side::Sell, Price::from_double(100.0, 0.01), 10, 1000000, OrderType::Limit);
    std::vector<TradeEvent> trades;
    EXPECT_TRUE(book->add(sell, trades));
    
    // FOK buy for exact quantity
    Order fok_buy(2, Side::Buy, Price::from_double(100.0, 0.01), 10, 1000001, OrderType::FOK);
    trades.clear();
    EXPECT_TRUE(book->add(fok_buy, trades));
    
    EXPECT_EQ(trades.size(), 1);
    EXPECT_EQ(trades[0].qty, 10);
    EXPECT_EQ(book->total_orders(), 0);
}

TEST_F(LimitBookTest, FOKOrderRejected) {
    // Add sell order with insufficient quantity
    Order sell(1, Side::Sell, Price::from_double(100.0, 0.01), 5, 1000000, OrderType::Limit);
    std::vector<TradeEvent> trades;
    EXPECT_TRUE(book->add(sell, trades));
    
    // FOK buy for more than available
    Order fok_buy(2, Side::Buy, Price::from_double(100.0, 0.01), 10, 1000001, OrderType::FOK);
    trades.clear();
    EXPECT_FALSE(book->add(fok_buy, trades)); // Should reject
    
    EXPECT_TRUE(trades.empty());
    EXPECT_EQ(book->total_orders(), 1); // Sell order still there
}

TEST_F(LimitBookTest, ReplaceOrder) {
    Order order(1, Side::Buy, Price::from_double(100.0, 0.01), 10, 1000000, OrderType::Limit);
    std::vector<TradeEvent> trades;
    EXPECT_TRUE(book->add(order, trades));
    
    ReplaceEvent replace_event;
    trades.clear();
    Price new_price = Price::from_double(101.0, 0.01);
    EXPECT_TRUE(book->replace(1, new_price, 15, replace_event, trades));
    
    EXPECT_EQ(replace_event.id, 1);
    EXPECT_EQ(replace_event.new_price.to_double(0.01), 101.0);
    EXPECT_EQ(replace_event.new_qty, 15);
    EXPECT_EQ(book->total_orders(), 1);
}

TEST_F(LimitBookTest, MultiLevelBook) {
    std::vector<TradeEvent> trades;
    
    // Add multiple buy levels
    EXPECT_TRUE(book->add(Order(1, Side::Buy, Price::from_double(100.0, 0.01), 10, 1000000), trades));
    EXPECT_TRUE(book->add(Order(2, Side::Buy, Price::from_double(99.0, 0.01), 10, 1000001), trades));
    EXPECT_TRUE(book->add(Order(3, Side::Buy, Price::from_double(98.0, 0.01), 10, 1000002), trades));
    
    // Add multiple sell levels
    EXPECT_TRUE(book->add(Order(4, Side::Sell, Price::from_double(101.0, 0.01), 10, 1000003), trades));
    EXPECT_TRUE(book->add(Order(5, Side::Sell, Price::from_double(102.0, 0.01), 10, 1000004), trades));
    
    BookTop top;
    EXPECT_TRUE(book->best_bid_ask(top));
    EXPECT_EQ(top.best_bid.to_double(0.01), 100.0);
    EXPECT_EQ(top.best_ask.to_double(0.01), 101.0);
    EXPECT_EQ(book->total_orders(), 5);
}
