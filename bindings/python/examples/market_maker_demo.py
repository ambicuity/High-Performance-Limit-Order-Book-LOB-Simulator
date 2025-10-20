#!/usr/bin/env python3
"""
Market Maker Demo for the High-Performance LOB Simulator

Demonstrates a simple market-making strategy with:
- Quoting on both sides of the market
- Inventory management
- Dynamic spread adjustment
"""

import sys
import os
from collections import defaultdict

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


class SimpleMarketMaker:
    """Simple market-making strategy with inventory management"""
    
    def __init__(self, engine, time_source, tick_size=0.01, 
                 spread_ticks=2, quote_size=10, max_inventory=100):
        self.engine = engine
        self.time_source = time_source
        self.tick_size = tick_size
        self.spread_ticks = spread_ticks
        self.quote_size = quote_size
        self.max_inventory = max_inventory
        
        self.next_order_id = 1000
        self.inventory = 0
        self.active_quotes = {}  # order_id -> (side, price, qty)
        self.mid_price = 100.0
        
        self.trades = []
        self.pnl = 0.0
        self.total_volume = 0
    
    def get_next_id(self):
        order_id = self.next_order_id
        self.next_order_id += 1
        return order_id
    
    def calculate_fair_price(self):
        """Calculate fair price (simplified - uses mid)"""
        top = self.engine.best_bid_ask()
        
        if top.best_bid.ticks != -1 and top.best_ask.ticks != -1:
            bid = top.best_bid.to_double(self.tick_size)
            ask = top.best_ask.to_double(self.tick_size)
            self.mid_price = (bid + ask) / 2.0
        
        return self.mid_price
    
    def calculate_skew(self):
        """Calculate price skew based on inventory"""
        # Skew quotes away from inventory position
        inventory_ratio = self.inventory / self.max_inventory
        skew_ticks = int(inventory_ratio * 5)  # Up to 5 ticks skew
        return skew_ticks
    
    def should_quote(self):
        """Determine if we should be quoting"""
        return abs(self.inventory) < self.max_inventory
    
    def cancel_all_quotes(self):
        """Cancel all active quotes"""
        for order_id in list(self.active_quotes.keys()):
            if self.engine.cancel(order_id):
                del self.active_quotes[order_id]
        
        # Drain cancel events
        self.engine.poll_events()
    
    def place_quotes(self):
        """Place new quotes on both sides"""
        fair_price = self.calculate_fair_price()
        skew = self.calculate_skew()
        
        # Calculate bid/ask prices with spread and skew
        bid_price = fair_price - (self.spread_ticks + skew) * self.tick_size
        ask_price = fair_price + (self.spread_ticks - skew) * self.tick_size
        
        # Adjust quote sizes based on inventory
        inventory_ratio = abs(self.inventory) / self.max_inventory
        size_reduction = max(0.5, 1.0 - inventory_ratio)
        
        bid_size = int(self.quote_size * size_reduction) if self.inventory < self.max_inventory else 0
        ask_size = int(self.quote_size * size_reduction) if self.inventory > -self.max_inventory else 0
        
        # Place bid
        if bid_size > 0:
            bid_id = self.get_next_id()
            bid_order = Order(
                bid_id, Side.Buy, 
                Price.from_double(bid_price, self.tick_size),
                bid_size, self.time_source.now_ns(), OrderType.Limit
            )
            if self.engine.submit(bid_order):
                self.active_quotes[bid_id] = (Side.Buy, bid_price, bid_size)
        
        # Place ask
        if ask_size > 0:
            ask_id = self.get_next_id()
            ask_order = Order(
                ask_id, Side.Sell,
                Price.from_double(ask_price, self.tick_size),
                ask_size, self.time_source.now_ns(), OrderType.Limit
            )
            if self.engine.submit(ask_order):
                self.active_quotes[ask_id] = (Side.Sell, ask_price, ask_size)
        
        # Drain accept events
        self.engine.poll_events()
    
    def process_events(self):
        """Process market events and update state"""
        events = self.engine.poll_events()
        
        for event in events:
            event_type = type(event).__name__
            
            if event_type == "TradeEvent":
                # Update inventory and PnL
                trade_price = event.price.to_double(self.tick_size)
                
                # Check if we were the maker
                if event.maker_id in self.active_quotes:
                    side, _, _ = self.active_quotes[event.maker_id]
                    
                    if side == Side.Buy:
                        # We bought (inventory increases)
                        self.inventory += event.qty
                        self.pnl -= trade_price * event.qty
                    else:
                        # We sold (inventory decreases)
                        self.inventory -= event.qty
                        self.pnl += trade_price * event.qty
                    
                    self.total_volume += event.qty
                    self.trades.append({
                        'side': 'BUY' if side == Side.Buy else 'SELL',
                        'price': trade_price,
                        'qty': event.qty,
                        'inventory': self.inventory
                    })
                    
                    # Remove from active quotes if fully filled
                    # (partial fills will be handled by book update)
                    
            elif event_type == "CancelEvent":
                if event.id in self.active_quotes:
                    del self.active_quotes[event.id]
    
    def quote_cycle(self):
        """One cycle of market making"""
        # Cancel existing quotes
        self.cancel_all_quotes()
        
        # Process any pending events
        self.process_events()
        
        # Place new quotes if appropriate
        if self.should_quote():
            self.place_quotes()
        
        # Advance time
        self.time_source.advance(1000000)  # 1ms
    
    def print_status(self):
        """Print current market maker status"""
        print(f"\nMarket Maker Status:")
        print(f"  Inventory: {self.inventory}")
        print(f"  Active Quotes: {len(self.active_quotes)}")
        print(f"  Total Volume: {self.total_volume}")
        print(f"  Realized PnL: ${self.pnl:.2f}")
        print(f"  Unrealized PnL: ${self.inventory * self.mid_price:.2f}")
        print(f"  Total PnL: ${self.pnl + self.inventory * self.mid_price:.2f}")


def main():
    print("=== Market Maker Demo ===\n")
    
    # Setup
    config = EngineConfig(max_orders=100000, ring_size=100000, tick_size=0.01)
    time_source = SimulatedTimeSource(1000000000)
    engine = MatchingEngine(config, time_source)
    
    # Create market maker
    mm = SimpleMarketMaker(
        engine, time_source, 
        tick_size=0.01, 
        spread_ticks=2,
        quote_size=10,
        max_inventory=100
    )
    
    # Seed the book with some initial orders
    print("Seeding initial market...")
    initial_orders = [
        Order(1, Side.Buy, Price.from_double(99.95, 0.01), 50, time_source.now_ns()),
        Order(2, Side.Buy, Price.from_double(99.90, 0.01), 50, time_source.now_ns()),
        Order(3, Side.Sell, Price.from_double(100.05, 0.01), 50, time_source.now_ns()),
        Order(4, Side.Sell, Price.from_double(100.10, 0.01), 50, time_source.now_ns()),
    ]
    
    for order in initial_orders:
        engine.submit(order)
        time_source.advance(1000)
    
    engine.poll_events()
    
    # Run market maker
    print("\nStarting market making...")
    num_cycles = 50
    
    for i in range(num_cycles):
        mm.quote_cycle()
        
        # Occasionally hit our quotes with aggressive orders
        if i % 10 == 5:
            # Aggressive buy
            agg_id = 5000 + i
            agg_buy = Order(
                agg_id, Side.Buy,
                Price.from_double(100.50, 0.01),
                5, time_source.now_ns(), OrderType.IOC
            )
            engine.submit(agg_buy)
        
        if i % 10 == 8:
            # Aggressive sell
            agg_id = 6000 + i
            agg_sell = Order(
                agg_id, Side.Sell,
                Price.from_double(99.50, 0.01),
                5, time_source.now_ns(), OrderType.IOC
            )
            engine.submit(agg_sell)
        
        # Print status periodically
        if (i + 1) % 10 == 0:
            print(f"\nCycle {i + 1}/{num_cycles}")
            mm.print_status()
    
    # Final summary
    print("\n" + "=" * 50)
    print("Market Making Session Complete")
    print("=" * 50)
    mm.print_status()
    
    if mm.trades:
        print(f"\nRecent Trades (last 10):")
        for trade in mm.trades[-10:]:
            print(f"  {trade['side']}: {trade['qty']} @ ${trade['price']:.2f} "
                  f"(inv: {trade['inventory']})")
    
    # Show final book
    top = engine.best_bid_ask()
    print(f"\nFinal Book Top:")
    print(f"  Best Bid: {top.best_bid.to_double(0.01):.2f} x {top.bid_qty}")
    print(f"  Best Ask: {top.best_ask.to_double(0.01):.2f} x {top.ask_qty}")
    
    print("\n=== Demo Complete ===")


if __name__ == "__main__":
    main()
