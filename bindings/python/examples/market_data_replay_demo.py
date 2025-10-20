#!/usr/bin/env python3
"""
Example demonstrating market data replay from CSV files.
Generates sample market data and then replays it through the engine.
"""

import csv
import os
from lobsim import (
    EngineConfig, MatchingEngine, Order, Price, Side, OrderType,
    SimulatedTimeSource, MarketDataReplay
)

def generate_sample_data(filename):
    """Generate sample market data CSV file."""
    print(f"Generating sample market data: {filename}")
    
    data = [
        # timestamp, action, order_id, side, price, qty, order_type
        [1000000, "ADD", 1, "BUY", 100.00, 50, "LIMIT"],
        [1001000, "ADD", 2, "BUY", 99.95, 75, "LIMIT"],
        [1002000, "ADD", 3, "SELL", 100.05, 60, "LIMIT"],
        [1003000, "ADD", 4, "SELL", 100.10, 80, "LIMIT"],
        [1004000, "ADD", 5, "BUY", 100.00, 30, "LIMIT"],  # Add to existing level
        [1005000, "ADD", 6, "BUY", 100.02, 100, "MARKET"],  # Market order - should trade
        [1006000, "CANCEL", 1, "BUY", 0, 0, "LIMIT"],
        [1007000, "ADD", 7, "SELL", 99.98, 40, "LIMIT"],  # Crossing order
        [1008000, "REPLACE", 2, "BUY", 99.90, 100, "LIMIT"],
        [1009000, "ADD", 8, "BUY", 99.85, 120, "IOC"],
    ]
    
    with open(filename, 'w', newline='') as f:
        writer = csv.writer(f)
        writer.writerow(["timestamp", "action", "order_id", "side", "price", "qty", "order_type"])
        writer.writerows(data)
    
    print(f"Generated {len(data)} market data messages\n")

def main():
    # Generate sample market data
    data_file = "/tmp/sample_market_data.csv"
    generate_sample_data(data_file)
    
    # Configure engine
    config = EngineConfig(max_orders=100000, ring_size=10000, tick_size=0.01)
    time_source = SimulatedTimeSource(0)
    engine = MatchingEngine(config, time_source)
    
    # Create replay engine
    replay = MarketDataReplay(engine)
    
    # Load market data
    print("="*60)
    print("Market Data Replay Demo")
    print("="*60 + "\n")
    
    if not replay.load_from_csv(data_file, 0.01):
        print("Error: Failed to load market data")
        return
    
    print(f"Loaded {replay.message_count()} messages from {data_file}\n")
    
    # Replay all messages
    print("="*60)
    print("Replaying All Messages:")
    print("="*60 + "\n")
    
    events = []
    processed = replay.replay_all(events)
    
    print(f"Processed {processed} messages")
    print(f"Generated {len(events)} events:\n")
    
    # Categorize events
    trades = []
    accepts = []
    cancels = []
    rejects = []
    
    for event in events:
        if hasattr(event, 'taker_id'):  # Trade event
            trades.append(event)
        elif hasattr(event, 'reason_code'):  # Reject event
            rejects.append(event)
        elif hasattr(event, 'remaining'):  # Cancel event
            cancels.append(event)
        elif hasattr(event, 'new_price'):  # Replace event
            # Skip replace events for simplicity
            pass
        else:  # Accept event
            accepts.append(event)
    
    # Display trades
    if trades:
        print(f"Trades ({len(trades)}):")
        for trade in trades:
            price = trade.price.to_double(0.01)
            print(f"  Order {trade.taker_id} x Order {trade.maker_id}: "
                  f"{trade.qty} @ ${price:.2f}")
    
    if accepts:
        print(f"\nAccepts ({len(accepts)}):")
        for accept in accepts[:5]:  # Show first 5
            print(f"  Order {accept.id} accepted")
        if len(accepts) > 5:
            print(f"  ... and {len(accepts) - 5} more")
    
    if cancels:
        print(f"\nCancels ({len(cancels)}):")
        for cancel in cancels:
            print(f"  Order {cancel.id} canceled (remaining: {cancel.remaining})")
    
    if rejects:
        print(f"\nRejects ({len(rejects)}):")
        for reject in rejects:
            print(f"  Order {reject.id} rejected (reason: {reject.reason_code})")
    
    # Show final book state
    print("\n" + "="*60)
    print("Final Order Book State:")
    print("="*60 + "\n")
    
    top = engine.best_bid_ask()
    bid_price = top.best_bid.to_double(0.01)
    ask_price = top.best_ask.to_double(0.01)
    
    print(f"Best Bid: ${bid_price:.2f} x {top.bid_qty}")
    print(f"Best Ask: ${ask_price:.2f} x {top.ask_qty}")
    print(f"Spread:   ${ask_price - bid_price:.2f}\n")
    
    # Get depth snapshot
    depth = engine.get_depth(5)
    
    print("Order Book Depth (5 levels):")
    print("\nASKS:")
    for level in reversed(depth.asks):
        price_val = level.price.to_double(0.01)
        print(f"  ${price_val:.2f} x {level.qty} ({level.order_count} orders)")
    
    print("\nBIDS:")
    for level in depth.bids:
        price_val = level.price.to_double(0.01)
        print(f"  ${price_val:.2f} x {level.qty} ({level.order_count} orders)")
    
    print("\n" + "="*60)
    print("Replay Complete")
    print("="*60)
    
    # Clean up
    if os.path.exists(data_file):
        os.remove(data_file)

if __name__ == "__main__":
    main()
