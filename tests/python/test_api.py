#!/usr/bin/env python3
"""
Python API tests for the High-Performance LOB Simulator
"""

import sys
import os

# Add parent directory to path
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '../../bindings/python'))

try:
    from lobsim import (
        EngineConfig, MatchingEngine, Order, Price, Side, OrderType,
        SimulatedTimeSource, TradeEvent, AcceptEvent, CancelEvent
    )
    LOBSIM_AVAILABLE = True
except ImportError as e:
    print(f"Warning: Could not import lobsim: {e}")
    LOBSIM_AVAILABLE = False


def test_engine_creation():
    """Test creating a matching engine"""
    if not LOBSIM_AVAILABLE:
        print("SKIP: test_engine_creation (lobsim not available)")
        return
    
    config = EngineConfig(1000, 1000, 0.01)
    time_source = SimulatedTimeSource(0)
    engine = MatchingEngine(config, time_source)
    
    assert engine is not None
    print("PASS: test_engine_creation")


def test_price_conversion():
    """Test price conversion between double and ticks"""
    if not LOBSIM_AVAILABLE:
        print("SKIP: test_price_conversion (lobsim not available)")
        return
    
    price = Price.from_double(100.50, 0.01)
    assert abs(price.to_double(0.01) - 100.50) < 0.001
    
    print("PASS: test_price_conversion")


def test_order_submission():
    """Test submitting orders"""
    if not LOBSIM_AVAILABLE:
        print("SKIP: test_order_submission (lobsim not available)")
        return
    
    config = EngineConfig(1000, 1000, 0.01)
    time_source = SimulatedTimeSource(1000000)
    engine = MatchingEngine(config, time_source)
    
    order = Order(1, Side.Buy, Price.from_double(100.0, 0.01), 
                 10, time_source.now_ns(), OrderType.Limit)
    
    result = engine.submit(order)
    assert result == True
    
    # Check for accept event
    events = engine.poll_events()
    assert len(events) > 0
    
    has_accept = any(type(e).__name__ == 'AcceptEvent' for e in events)
    assert has_accept
    
    print("PASS: test_order_submission")


def test_matching():
    """Test order matching"""
    if not LOBSIM_AVAILABLE:
        print("SKIP: test_matching (lobsim not available)")
        return
    
    config = EngineConfig(1000, 1000, 0.01)
    time_source = SimulatedTimeSource(1000000)
    engine = MatchingEngine(config, time_source)
    
    # Submit sell order
    sell = Order(1, Side.Sell, Price.from_double(100.0, 0.01),
                10, time_source.now_ns(), OrderType.Limit)
    engine.submit(sell)
    engine.poll_events()  # Clear events
    
    # Submit matching buy order
    buy = Order(2, Side.Buy, Price.from_double(100.0, 0.01),
               10, time_source.now_ns(), OrderType.Limit)
    engine.submit(buy)
    
    # Check for trade event
    events = engine.poll_events()
    trade_events = [e for e in events if type(e).__name__ == 'TradeEvent']
    
    assert len(trade_events) > 0
    trade = trade_events[0]
    assert trade.qty == 10
    
    print("PASS: test_matching")


def test_cancel():
    """Test order cancellation"""
    if not LOBSIM_AVAILABLE:
        print("SKIP: test_cancel (lobsim not available)")
        return
    
    config = EngineConfig(1000, 1000, 0.01)
    time_source = SimulatedTimeSource(1000000)
    engine = MatchingEngine(config, time_source)
    
    # Submit order
    order = Order(1, Side.Buy, Price.from_double(100.0, 0.01),
                 10, time_source.now_ns(), OrderType.Limit)
    engine.submit(order)
    engine.poll_events()
    
    # Cancel it
    result = engine.cancel(1)
    assert result == True
    
    # Check for cancel event
    events = engine.poll_events()
    cancel_events = [e for e in events if type(e).__name__ == 'CancelEvent']
    
    assert len(cancel_events) > 0
    cancel = cancel_events[0]
    assert cancel.id == 1
    assert cancel.remaining == 10
    
    print("PASS: test_cancel")


def test_replace():
    """Test order replacement"""
    if not LOBSIM_AVAILABLE:
        print("SKIP: test_replace (lobsim not available)")
        return
    
    config = EngineConfig(1000, 1000, 0.01)
    time_source = SimulatedTimeSource(1000000)
    engine = MatchingEngine(config, time_source)
    
    # Submit order
    order = Order(1, Side.Buy, Price.from_double(100.0, 0.01),
                 10, time_source.now_ns(), OrderType.Limit)
    engine.submit(order)
    engine.poll_events()
    
    # Replace it
    new_price = Price.from_double(101.0, 0.01)
    result = engine.replace(1, new_price, 20)
    assert result == True
    
    # Check for replace event
    events = engine.poll_events()
    replace_events = [e for e in events if type(e).__name__ == 'ReplaceEvent']
    
    assert len(replace_events) > 0
    replace = replace_events[0]
    assert replace.id == 1
    assert abs(replace.new_price.to_double(0.01) - 101.0) < 0.001
    assert replace.new_qty == 20
    
    print("PASS: test_replace")


def test_book_top():
    """Test getting best bid/ask"""
    if not LOBSIM_AVAILABLE:
        print("SKIP: test_book_top (lobsim not available)")
        return
    
    config = EngineConfig(1000, 1000, 0.01)
    time_source = SimulatedTimeSource(1000000)
    engine = MatchingEngine(config, time_source)
    
    # Submit orders
    buy = Order(1, Side.Buy, Price.from_double(100.0, 0.01),
               10, time_source.now_ns(), OrderType.Limit)
    sell = Order(2, Side.Sell, Price.from_double(101.0, 0.01),
                15, time_source.now_ns(), OrderType.Limit)
    
    engine.submit(buy)
    engine.submit(sell)
    
    # Get book top
    top = engine.best_bid_ask()
    
    assert abs(top.best_bid.to_double(0.01) - 100.0) < 0.001
    assert top.bid_qty == 10
    assert abs(top.best_ask.to_double(0.01) - 101.0) < 0.001
    assert top.ask_qty == 15
    
    print("PASS: test_book_top")


def test_time_source():
    """Test simulated time source"""
    if not LOBSIM_AVAILABLE:
        print("SKIP: test_time_source (lobsim not available)")
        return
    
    time_source = SimulatedTimeSource(1000)
    
    t1 = time_source.now_ns()
    assert t1 == 1000
    
    time_source.advance(500)
    t2 = time_source.now_ns()
    assert t2 == 1500
    
    time_source.set(2000)
    t3 = time_source.now_ns()
    assert t3 == 2000
    
    print("PASS: test_time_source")


def test_ioc_order():
    """Test IOC order type"""
    if not LOBSIM_AVAILABLE:
        print("SKIP: test_ioc_order (lobsim not available)")
        return
    
    config = EngineConfig(1000, 1000, 0.01)
    time_source = SimulatedTimeSource(1000000)
    engine = MatchingEngine(config, time_source)
    
    # Submit limit sell
    sell = Order(1, Side.Sell, Price.from_double(100.0, 0.01),
                5, time_source.now_ns(), OrderType.Limit)
    engine.submit(sell)
    engine.poll_events()
    
    # Submit IOC buy for more quantity
    ioc_buy = Order(2, Side.Buy, Price.from_double(100.0, 0.01),
                   10, time_source.now_ns(), OrderType.IOC)
    engine.submit(ioc_buy)
    
    events = engine.poll_events()
    trade_events = [e for e in events if type(e).__name__ == 'TradeEvent']
    
    # Should only fill 5
    assert len(trade_events) == 1
    assert trade_events[0].qty == 5
    
    print("PASS: test_ioc_order")


def test_determinism():
    """Test that same inputs produce same outputs"""
    if not LOBSIM_AVAILABLE:
        print("SKIP: test_determinism (lobsim not available)")
        return
    
    def run_scenario():
        config = EngineConfig(1000, 1000, 0.01)
        time_source = SimulatedTimeSource(1000000)
        engine = MatchingEngine(config, time_source)
        
        # Submit sequence of orders
        orders = [
            Order(1, Side.Buy, Price.from_double(100.0, 0.01), 10, 1000000),
            Order(2, Side.Sell, Price.from_double(101.0, 0.01), 10, 1000001),
            Order(3, Side.Buy, Price.from_double(100.5, 0.01), 5, 1000002),
            Order(4, Side.Sell, Price.from_double(100.5, 0.01), 5, 1000003),
        ]
        
        for order in orders:
            engine.submit(order)
        
        events = engine.poll_events()
        trade_events = [e for e in events if type(e).__name__ == 'TradeEvent']
        
        return len(trade_events), engine.best_bid_ask()
    
    # Run twice
    result1 = run_scenario()
    result2 = run_scenario()
    
    # Should be identical
    assert result1[0] == result2[0]
    assert result1[1].best_bid.ticks == result2[1].best_bid.ticks
    assert result1[1].best_ask.ticks == result2[1].best_ask.ticks
    
    print("PASS: test_determinism")


def main():
    """Run all tests"""
    print("=== Python API Tests ===\n")
    
    if not LOBSIM_AVAILABLE:
        print("ERROR: lobsim module not available. Build the project first:")
        print("  mkdir build && cd build")
        print("  cmake .. && make")
        return 1
    
    tests = [
        test_engine_creation,
        test_price_conversion,
        test_order_submission,
        test_matching,
        test_cancel,
        test_replace,
        test_book_top,
        test_time_source,
        test_ioc_order,
        test_determinism,
    ]
    
    passed = 0
    failed = 0
    
    for test in tests:
        try:
            test()
            passed += 1
        except AssertionError as e:
            print(f"FAIL: {test.__name__}: {e}")
            failed += 1
        except Exception as e:
            print(f"ERROR: {test.__name__}: {e}")
            failed += 1
    
    print(f"\n=== Results ===")
    print(f"Passed: {passed}")
    print(f"Failed: {failed}")
    
    return 0 if failed == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
