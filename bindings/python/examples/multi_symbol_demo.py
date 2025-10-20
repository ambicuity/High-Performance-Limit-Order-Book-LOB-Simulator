#!/usr/bin/env python3
"""
Example demonstrating multi-symbol support with separate order books.
"""

from lobsim import (
    EngineConfig, MatchingEngine, Order, Price, Side, OrderType,
    SimulatedTimeSource, MultiSymbolEngine
)

def main():
    # Configure multi-symbol engine
    config = EngineConfig(max_orders=100000, ring_size=10000, tick_size=0.01)
    time_source = SimulatedTimeSource(0)
    multi_engine = MultiSymbolEngine(config, time_source)
    
    # Add multiple symbols
    symbols = ["AAPL", "GOOGL", "MSFT"]
    
    print("="*60)
    print("Multi-Symbol Order Book Simulation")
    print("="*60)
    
    for symbol in symbols:
        multi_engine.add_symbol(symbol)
        print(f"Added symbol: {symbol}")
    
    # Submit orders to different symbols
    print("\n" + "="*60)
    print("Submitting Orders:")
    print("="*60 + "\n")
    
    # AAPL orders
    print("AAPL:")
    multi_engine.submit("AAPL", Order(1, Side.Buy, Price.from_double(150.00, 0.01), 
                                       100, time_source.now_ns(), OrderType.Limit))
    print("  BUY  100 @ $150.00")
    
    multi_engine.submit("AAPL", Order(2, Side.Sell, Price.from_double(150.10, 0.01), 
                                       100, time_source.now_ns(), OrderType.Limit))
    print("  SELL 100 @ $150.10")
    
    # GOOGL orders
    print("\nGOOGL:")
    multi_engine.submit("GOOGL", Order(3, Side.Buy, Price.from_double(2800.00, 0.01), 
                                        50, time_source.now_ns(), OrderType.Limit))
    print("  BUY  50 @ $2800.00")
    
    multi_engine.submit("GOOGL", Order(4, Side.Sell, Price.from_double(2800.50, 0.01), 
                                        50, time_source.now_ns(), OrderType.Limit))
    print("  SELL 50 @ $2800.50")
    
    # MSFT orders
    print("\nMSFT:")
    multi_engine.submit("MSFT", Order(5, Side.Buy, Price.from_double(300.00, 0.01), 
                                       75, time_source.now_ns(), OrderType.Limit))
    print("  BUY  75 @ $300.00")
    
    multi_engine.submit("MSFT", Order(6, Side.Sell, Price.from_double(300.05, 0.01), 
                                       75, time_source.now_ns(), OrderType.Limit))
    print("  SELL 75 @ $300.05")
    
    # Display market data for all symbols
    print("\n" + "="*60)
    print("Current Market Data:")
    print("="*60)
    print(f"\n{'Symbol':<10} {'Bid':<12} {'Bid Qty':<10} {'Ask':<12} {'Ask Qty':<10} {'Spread':<10}")
    print("-" * 64)
    
    for symbol in symbols:
        top = multi_engine.best_bid_ask(symbol)
        bid_price = top.best_bid.to_double(0.01)
        ask_price = top.best_ask.to_double(0.01)
        spread = ask_price - bid_price
        
        print(f"{symbol:<10} ${bid_price:<11.2f} {top.bid_qty:<10} "
              f"${ask_price:<11.2f} {top.ask_qty:<10} ${spread:<9.2f}")
    
    # Simulate a trade by submitting crossing order
    print("\n" + "="*60)
    print("Executing Market Order on AAPL:")
    print("="*60 + "\n")
    
    # Buy at market - should cross with best ask
    multi_engine.submit("AAPL", Order(7, Side.Buy, Price.from_double(151.00, 0.01), 
                                       50, time_source.now_ns(), OrderType.Market))
    print("Submitted: BUY 50 @ MARKET")
    
    # Poll events for AAPL
    events = multi_engine.poll_events("AAPL")
    print(f"\nGenerated {len(events)} events:")
    for event in events:
        if hasattr(event, 'taker_id'):  # Trade event
            price = event.price.to_double(0.01)
            print(f"  TRADE: {event.qty} @ ${price:.2f} (taker: {event.taker_id}, maker: {event.maker_id})")
    
    # Display updated market data
    print("\n" + "="*60)
    print("Updated Market Data After Trade:")
    print("="*60)
    print(f"\n{'Symbol':<10} {'Bid':<12} {'Bid Qty':<10} {'Ask':<12} {'Ask Qty':<10} {'Spread':<10}")
    print("-" * 64)
    
    for symbol in symbols:
        top = multi_engine.best_bid_ask(symbol)
        bid_price = top.best_bid.to_double(0.01)
        ask_price = top.best_ask.to_double(0.01)
        spread = ask_price - bid_price
        
        print(f"{symbol:<10} ${bid_price:<11.2f} {top.bid_qty:<10} "
              f"${ask_price:<11.2f} {top.ask_qty:<10} ${spread:<9.2f}")
    
    # Get depth for one symbol
    print("\n" + "="*60)
    print("AAPL Order Book Depth:")
    print("="*60)
    
    depth = multi_engine.get_depth("AAPL", 5)
    
    print("\nASKS:")
    for level in reversed(depth.asks):
        price_val = level.price.to_double(0.01)
        print(f"  ${price_val:.2f} x {level.qty}")
    
    print("\nBIDS:")
    for level in depth.bids:
        price_val = level.price.to_double(0.01)
        print(f"  ${price_val:.2f} x {level.qty}")
    
    # List all active symbols
    print("\n" + "="*60)
    print(f"Active Symbols: {', '.join(multi_engine.get_symbols())}")
    print("="*60)

if __name__ == "__main__":
    main()
