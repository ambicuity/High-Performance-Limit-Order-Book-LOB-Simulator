"""
High-Performance Limit Order Book (LOB) Simulator

A production-grade matching engine with Python bindings for strategy testing,
market-making simulations, and order flow analysis.
"""

try:
    from .lobsim import (
        # Enums
        Side,
        OrderType,
        EventType,
        
        # Core types
        Price,
        Order,
        
        # Events
        TradeEvent,
        AcceptEvent,
        RejectEvent,
        CancelEvent,
        ReplaceEvent,
        BookTop,
        
        # Configuration
        EngineConfig,
        
        # Time sources
        TimeSource,
        SimulatedTimeSource,
        RealTimeSource,
        
        # Engine
        MatchingEngine,
        
        # Market data
        MDOrder,
        MarketDataFeed,
    )
except ImportError as e:
    import warnings
    warnings.warn(f"Could not import C++ extension: {e}. Please build the project first.")
    # Define stub classes for development
    class Side:
        Buy = 0
        Sell = 1
    
    class OrderType:
        Limit = 0
        Market = 1
        IOC = 2
        FOK = 3

__version__ = "1.0.0"
__author__ = "Ritesh Rana"

__all__ = [
    "Side",
    "OrderType",
    "EventType",
    "Price",
    "Order",
    "TradeEvent",
    "AcceptEvent",
    "RejectEvent",
    "CancelEvent",
    "ReplaceEvent",
    "BookTop",
    "EngineConfig",
    "TimeSource",
    "SimulatedTimeSource",
    "RealTimeSource",
    "MatchingEngine",
    "MDOrder",
    "MarketDataFeed",
]
