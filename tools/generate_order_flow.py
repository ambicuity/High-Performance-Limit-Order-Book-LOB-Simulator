#!/usr/bin/env python3
"""
Generate synthetic order flow for testing and benchmarking

Creates CSV files with realistic order patterns including:
- Limit orders at various price levels
- Market orders
- Cancellations
- Order replacements
"""

import random
import argparse
import csv
from pathlib import Path


class OrderFlowGenerator:
    """Generate synthetic order flow with configurable patterns"""
    
    def __init__(self, seed=42, base_price=100.0, tick_size=0.01):
        self.rng = random.Random(seed)
        self.base_price = base_price
        self.tick_size = tick_size
        self.next_order_id = 1
        self.active_orders = []
        self.timestamp_ns = 1000000000  # Start at 1 second
    
    def get_next_id(self):
        order_id = self.next_order_id
        self.next_order_id += 1
        return order_id
    
    def advance_time(self, mean_ns=1000000, std_ns=500000):
        """Advance timestamp with jitter"""
        delta = max(100, int(self.rng.gauss(mean_ns, std_ns)))
        self.timestamp_ns += delta
        return self.timestamp_ns
    
    def generate_price_offset(self, volatility=1.0):
        """Generate price offset in ticks"""
        # Use exponential distribution for realistic price clustering
        offset_ticks = int(self.rng.expovariate(1.0 / (5.0 * volatility)))
        return offset_ticks
    
    def generate_limit_order(self, side_bias=0.0):
        """Generate a limit order
        
        Args:
            side_bias: Positive values favor buys, negative favors sells
        """
        order_id = self.get_next_id()
        
        # Determine side (with bias)
        side = 'buy' if self.rng.random() + side_bias > 0.5 else 'sell'
        
        # Generate price with clustering near mid
        offset_ticks = self.generate_price_offset()
        
        if side == 'buy':
            price = self.base_price - offset_ticks * self.tick_size
        else:
            price = self.base_price + offset_ticks * self.tick_size
        
        # Quantity - power law distribution
        qty = int(self.rng.paretovariate(1.5) * 10)
        qty = max(1, min(qty, 1000))  # Clamp between 1 and 1000
        
        order = {
            'ts_ns': self.timestamp_ns,
            'order_id': order_id,
            'side': side,
            'px': round(price, 2),
            'qty': qty,
            'type': 'limit'
        }
        
        self.active_orders.append(order_id)
        self.advance_time()
        
        return order
    
    def generate_market_order(self):
        """Generate a market order"""
        order_id = self.get_next_id()
        side = 'buy' if self.rng.random() > 0.5 else 'sell'
        qty = int(self.rng.paretovariate(1.5) * 10)
        qty = max(1, min(qty, 500))
        
        order = {
            'ts_ns': self.timestamp_ns,
            'order_id': order_id,
            'side': side,
            'px': 0.0,  # Market orders don't have a price
            'qty': qty,
            'type': 'market'
        }
        
        self.advance_time()
        return order
    
    def generate_ioc_order(self):
        """Generate an IOC order"""
        order_id = self.get_next_id()
        side = 'buy' if self.rng.random() > 0.5 else 'sell'
        
        offset_ticks = self.generate_price_offset(volatility=0.5)
        if side == 'buy':
            price = self.base_price + offset_ticks * self.tick_size  # Aggressive
        else:
            price = self.base_price - offset_ticks * self.tick_size  # Aggressive
        
        qty = int(self.rng.paretovariate(1.5) * 10)
        qty = max(1, min(qty, 200))
        
        order = {
            'ts_ns': self.timestamp_ns,
            'order_id': order_id,
            'side': side,
            'px': round(price, 2),
            'qty': qty,
            'type': 'ioc'
        }
        
        self.advance_time()
        return order
    
    def generate_cancel(self):
        """Generate a cancel for an active order"""
        if not self.active_orders:
            return None
        
        # Cancel a random active order
        order_id = self.rng.choice(self.active_orders)
        self.active_orders.remove(order_id)
        
        cancel = {
            'ts_ns': self.timestamp_ns,
            'order_id': order_id,
            'side': 'buy',  # Doesn't matter for cancel
            'px': 0.0,
            'qty': 0,
            'type': 'cancel'
        }
        
        self.advance_time()
        return cancel
    
    def generate_replace(self):
        """Generate a replace for an active order"""
        if not self.active_orders:
            return None
        
        order_id = self.rng.choice(self.active_orders)
        side = 'buy' if self.rng.random() > 0.5 else 'sell'
        
        # New price
        offset_ticks = self.generate_price_offset()
        if side == 'buy':
            price = self.base_price - offset_ticks * self.tick_size
        else:
            price = self.base_price + offset_ticks * self.tick_size
        
        # New quantity
        qty = int(self.rng.paretovariate(1.5) * 10)
        qty = max(1, min(qty, 1000))
        
        # New price and qty
        new_price = round(price, 2)
        new_qty = qty
        
        replace = {
            'ts_ns': self.timestamp_ns,
            'order_id': order_id,
            'side': side,
            'px': round(price, 2),
            'qty': qty,
            'type': 'replace',
            'new_px': new_price,
            'new_qty': new_qty
        }
        
        self.advance_time()
        return replace
    
    def generate_order_flow(self, num_orders=10000, 
                           market_pct=5, ioc_pct=10, 
                           cancel_pct=20, replace_pct=5):
        """Generate a complete order flow
        
        Args:
            num_orders: Total number of order events
            market_pct: Percentage of market orders
            ioc_pct: Percentage of IOC orders
            cancel_pct: Percentage of cancels
            replace_pct: Percentage of replaces
        """
        orders = []
        
        # First, generate some initial limit orders to seed the book
        for _ in range(100):
            orders.append(self.generate_limit_order())
        
        for i in range(num_orders):
            rand = self.rng.random() * 100
            
            if rand < cancel_pct and self.active_orders:
                order = self.generate_cancel()
            elif rand < cancel_pct + replace_pct and self.active_orders:
                order = self.generate_replace()
            elif rand < cancel_pct + replace_pct + market_pct:
                order = self.generate_market_order()
            elif rand < cancel_pct + replace_pct + market_pct + ioc_pct:
                order = self.generate_ioc_order()
            else:
                order = self.generate_limit_order()
            
            if order:
                orders.append(order)
        
        return orders
    
    def save_to_csv(self, orders, filename):
        """Save orders to CSV file"""
        with open(filename, 'w', newline='') as f:
            fieldnames = ['ts_ns', 'order_id', 'side', 'px', 'qty', 'type', 'new_px', 'new_qty']
            writer = csv.DictWriter(f, fieldnames=fieldnames)
            
            writer.writeheader()
            for order in orders:
                # Fill in missing fields
                if 'new_px' not in order:
                    order['new_px'] = ''
                if 'new_qty' not in order:
                    order['new_qty'] = ''
                writer.writerow(order)
        
        print(f"Saved {len(orders)} orders to {filename}")


def main():
    parser = argparse.ArgumentParser(description='Generate synthetic order flow')
    parser.add_argument('-n', '--num-orders', type=int, default=10000,
                       help='Number of orders to generate (default: 10000)')
    parser.add_argument('-o', '--output', type=str, default='order_flow.csv',
                       help='Output CSV file (default: order_flow.csv)')
    parser.add_argument('-s', '--seed', type=int, default=42,
                       help='Random seed (default: 42)')
    parser.add_argument('--base-price', type=float, default=100.0,
                       help='Base price (default: 100.0)')
    parser.add_argument('--tick-size', type=float, default=0.01,
                       help='Tick size (default: 0.01)')
    parser.add_argument('--market-pct', type=float, default=5.0,
                       help='Percentage of market orders (default: 5.0)')
    parser.add_argument('--ioc-pct', type=float, default=10.0,
                       help='Percentage of IOC orders (default: 10.0)')
    parser.add_argument('--cancel-pct', type=float, default=20.0,
                       help='Percentage of cancels (default: 20.0)')
    parser.add_argument('--replace-pct', type=float, default=5.0,
                       help='Percentage of replaces (default: 5.0)')
    
    args = parser.parse_args()
    
    print(f"=== Order Flow Generator ===")
    print(f"Generating {args.num_orders} orders...")
    print(f"Base price: {args.base_price}, Tick size: {args.tick_size}")
    print(f"Market: {args.market_pct}%, IOC: {args.ioc_pct}%, "
          f"Cancel: {args.cancel_pct}%, Replace: {args.replace_pct}%")
    
    generator = OrderFlowGenerator(
        seed=args.seed,
        base_price=args.base_price,
        tick_size=args.tick_size
    )
    
    orders = generator.generate_order_flow(
        num_orders=args.num_orders,
        market_pct=args.market_pct,
        ioc_pct=args.ioc_pct,
        cancel_pct=args.cancel_pct,
        replace_pct=args.replace_pct
    )
    
    generator.save_to_csv(orders, args.output)
    print(f"\nGeneration complete!")


if __name__ == "__main__":
    main()
