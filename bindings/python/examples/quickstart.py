#!/usr/bin/env python3
"""
Quickstart example for the High-Performance LOB Simulator

Demonstrates basic operations: submitting orders, matching, and retrieving events.
"""

import sys
import os

# Add parent directory to path for local development
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))

try:
    from lobsim import (
        EngineConfig, MatchingEngine, Order, Price, Side, OrderType,
        SimulatedTimeSource, EventType
    )
except ImportError:
    print("Error: Could not import lobsim. Please build the project first:")
    print("  mkdir build && cd build")
    print("  cmake .. && make")
    sys.exit(1)


def main():
    print("=== High-Performance LOB Simulator - Quickstart ===\n")
    
    # Create configuration
    config = EngineConfig(
        max_orders=10000,
        ring_size=10000,
        tick_size=0.01
    )
    
    # Create simulated time source for deterministic testing
    time_source = SimulatedTimeSource(1000000000)  # Start at 1 second
    
    # Create matching engine
    engine = MatchingEngine(config, time_source)
    print(f"Created matching engine with tick size: {config.tick_size}")
    print(f"Maximum orders: {config.max_orders}\n")
    
    # Submit some orders
    print("Submitting orders...")
    
    # Sell orders
    sell1 = Order(1, Side.Sell, Price.from_double(100.50, 0.01), 100, time_source.now_ns(), OrderType.Limit)
    sell2 = Order(2, Side.Sell, Price.from_double(100.75, 0.01), 50, time_source.now_ns(), OrderType.Limit)
    
    engine.submit(sell1)
    time_source.advance(1000)
    
    engine.submit(sell2)
    time_source.advance(1000)
    
    # Buy orders
    buy1 = Order(3, Side.Buy, Price.from_double(100.25, 0.01), 80, time_source.now_ns(), OrderType.Limit)
    buy2 = Order(4, Side.Buy, Price.from_double(100.00, 0.01), 60, time_source.now_ns(), OrderType.Limit)
    
    engine.submit(buy1)
    time_source.advance(1000)
    
    engine.submit(buy2)
    time_source.advance(1000)
    
    # Check book top
    top = engine.best_bid_ask()
    print(f"\nBook Top:")
    print(f"  Best Bid: {top.best_bid.to_double(0.01):.2f} x {top.bid_qty}")
    print(f"  Best Ask: {top.best_ask.to_double(0.01):.2f} x {top.ask_qty}")
    
    # Submit a crossing order to generate trades
    print(f"\nSubmitting crossing buy order at 100.50...")
    cross_buy = Order(5, Side.Buy, Price.from_double(100.50, 0.01), 75, time_source.now_ns(), OrderType.Limit)
    engine.submit(cross_buy)
    time_source.advance(1000)
    
    # Poll events
    events = engine.poll_events()
    print(f"\nReceived {len(events)} events:")
    
    trade_count = 0
    for event in events:
        event_type = type(event).__name__
        
        if event_type == "TradeEvent":
            trade_count += 1
            price = event.price.to_double(0.01)
            print(f"  TRADE: {event.qty} @ {price:.2f} (Taker: {event.taker_id}, Maker: {event.maker_id})")
        elif event_type == "AcceptEvent":
            print(f"  ACCEPT: Order {event.id}")
        elif event_type == "BookTop":
            print(f"  BOOK UPDATE: Bid {event.best_bid.to_double(0.01):.2f} x {event.bid_qty}, "
                  f"Ask {event.best_ask.to_double(0.01):.2f} x {event.ask_qty}")
    
    print(f"\nTotal trades: {trade_count}")
    
    # Final book state
    final_top = engine.best_bid_ask()
    print(f"\nFinal Book Top:")
    print(f"  Best Bid: {final_top.best_bid.to_double(0.01):.2f} x {final_top.bid_qty}")
    print(f"  Best Ask: {final_top.best_ask.to_double(0.01):.2f} x {final_top.ask_qty}")
    
    # Test cancel
    print(f"\nCanceling order 4...")
    if engine.cancel(4):
        print("  Cancel successful")
        events = engine.poll_events()
        for event in events:
            if type(event).__name__ == "CancelEvent":
                print(f"  Canceled order {event.id}, remaining qty: {event.remaining}")
    
    # Test replace
    print(f"\nReplacing order 3 with new price and quantity...")
    new_price = Price.from_double(100.35, 0.01)
    if engine.replace(3, new_price, 100):
        print("  Replace successful")
        events = engine.poll_events()
        for event in events:
            if type(event).__name__ == "ReplaceEvent":
                print(f"  Replaced order {event.id}, new price: {event.new_price.to_double(0.01):.2f}, "
                      f"new qty: {event.new_qty}")
    
    print("\n=== Quickstart Complete ===")


if __name__ == "__main__":
    main()
