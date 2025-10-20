#!/usr/bin/env python3
"""
Example demonstrating order book depth snapshots at multiple levels.
"""

from lobsim import (
    EngineConfig, MatchingEngine, Order, Price, Side, OrderType,
    SimulatedTimeSource
)

def main():
    # Configure engine
    config = EngineConfig(max_orders=100000, ring_size=10000, tick_size=0.01)
    time_source = SimulatedTimeSource(0)
    engine = MatchingEngine(config, time_source)
    
    # Build a multi-level order book
    print("Building order book with multiple price levels...\n")
    
    # Add buy orders at different price levels
    buy_orders = [
        (1, 100.00, 50),   # Best bid
        (2, 99.95, 75),
        (3, 99.90, 100),
        (4, 99.85, 120),
        (5, 99.80, 150),
    ]
    
    for order_id, price, qty in buy_orders:
        order = Order(order_id, Side.Buy, Price.from_double(price, 0.01), 
                     qty, time_source.now_ns(), OrderType.Limit)
        engine.submit(order)
        print(f"Submitted BUY  order {order_id}: {qty} @ ${price:.2f}")
    
    # Add sell orders at different price levels
    sell_orders = [
        (11, 100.05, 60),  # Best ask
        (12, 100.10, 80),
        (13, 100.15, 110),
        (14, 100.20, 130),
        (15, 100.25, 140),
    ]
    
    for order_id, price, qty in sell_orders:
        order = Order(order_id, Side.Sell, Price.from_double(price, 0.01), 
                     qty, time_source.now_ns(), OrderType.Limit)
        engine.submit(order)
        print(f"Submitted SELL order {order_id}: {qty} @ ${price:.2f}")
    
    # Get depth snapshot at 3 levels
    print("\n" + "="*60)
    print("Order Book Depth Snapshot (3 levels):")
    print("="*60)
    
    depth = engine.get_depth(3)
    
    print("\nASKS (Sell Side):")
    print(f"{'Price':<12} {'Quantity':<12} {'Orders':<10}")
    print("-" * 35)
    for level in reversed(depth.asks):
        price_val = level.price.to_double(0.01)
        print(f"${price_val:<11.2f} {level.qty:<12} {level.order_count:<10}")
    
    print("\n" + "-" * 35)
    
    # Show best bid/ask
    top = engine.best_bid_ask()
    bid_price = top.best_bid.to_double(0.01)
    ask_price = top.best_ask.to_double(0.01)
    spread = ask_price - bid_price
    print(f"SPREAD: ${spread:.2f} ({spread/bid_price*100:.3f}%)")
    print("-" * 35 + "\n")
    
    print("BIDS (Buy Side):")
    print(f"{'Price':<12} {'Quantity':<12} {'Orders':<10}")
    print("-" * 35)
    for level in depth.bids:
        price_val = level.price.to_double(0.01)
        print(f"${price_val:<11.2f} {level.qty:<12} {level.order_count:<10}")
    
    # Get full depth snapshot (5 levels)
    print("\n" + "="*60)
    print("Full Order Book Depth (5 levels):")
    print("="*60)
    
    depth_full = engine.get_depth(5)
    
    print("\nASKS:")
    for level in reversed(depth_full.asks):
        price_val = level.price.to_double(0.01)
        print(f"  ${price_val:.2f} x {level.qty} ({level.order_count} orders)")
    
    print(f"\n  --- SPREAD: ${spread:.2f} ---\n")
    
    print("BIDS:")
    for level in depth_full.bids:
        price_val = level.price.to_double(0.01)
        print(f"  ${price_val:.2f} x {level.qty} ({level.order_count} orders)")
    
    # Calculate total liquidity
    total_bid_qty = sum(level.qty for level in depth_full.bids)
    total_ask_qty = sum(level.qty for level in depth_full.asks)
    
    print("\n" + "="*60)
    print(f"Total Liquidity (5 levels):")
    print(f"  Bids: {total_bid_qty} units")
    print(f"  Asks: {total_ask_qty} units")
    print(f"  Total: {total_bid_qty + total_ask_qty} units")
    print("="*60)

if __name__ == "__main__":
    main()
