# High-Performance Limit Order Book (LOB) Simulator

[![CI](https://github.com/ambicuity/High-Performance-Limit-Order-Book-LOB-Simulator/workflows/CI/badge.svg)](https://github.com/ambicuity/High-Performance-Limit-Order-Book-LOB-Simulator/actions)

A production-grade matching engine and limit order book implementation in C++20 with Python bindings. Designed for ultra-low latency, deterministic simulation, and strategy backtesting with a focus on market-making algorithms.

**Author:** Ritesh Rana

## Features

- **High Performance**: Sub-microsecond average latency for order operations
- **Zero-Copy Design**: Custom memory pools eliminate heap allocations on hot paths
- **Deterministic**: Fully reproducible simulations with injectable time sources
- **Price-Time Priority**: Strict FIFO matching at each price level
- **Multiple Order Types**: Limit, Market, IOC (Immediate-Or-Cancel), FOK (Fill-Or-Kill)
- **Iceberg Orders**: Hidden order quantities with visible display amounts
- **Pegged Orders**: Orders pegged to mid price, best bid, or best ask
- **Multi-Symbol Support**: Trade multiple symbols with independent order books
- **Market Depth Snapshots**: Get order book depth at configurable levels
- **Market Data Replay**: Replay historical order flow from CSV files
- **WebSocket Feed**: Real-time market data streaming for visualization
- **Lock-Free Communication**: SPSC ring buffers for event streaming
- **Python Bindings**: Ergonomic API with pybind11 for strategy development
- **Comprehensive Testing**: Unit tests, integration tests, and benchmarks

## Architecture

### Core Components

```
┌─────────────────────────────────────────────────────┐
│                  MatchingEngine                     │
│  ┌───────────────┐         ┌──────────────────┐   │
│  │  LimitBook    │◄────────┤   TimeSource     │   │
│  │  ┌─────────┐  │         └──────────────────┘   │
│  │  │ Bids    │  │         ┌──────────────────┐   │
│  │  │ (Price→ │  │◄────────┤  OrderPool       │   │
│  │  │  FIFO)  │  │         └──────────────────┘   │
│  │  ├─────────┤  │                                 │
│  │  │ Asks    │  │         ┌──────────────────┐   │
│  │  │ (Price→ │  │────────►│  RingBuffer      │   │
│  │  │  FIFO)  │  │         │  (Events)        │   │
│  │  └─────────┘  │         └──────────────────┘   │
│  └───────────────┘                                 │
└─────────────────────────────────────────────────────┘
```

### Data Structures

- **Price**: Integer-tick representation to avoid floating-point issues
- **BookLevel**: Intrusive FIFO queue of orders at a single price level
- **LimitBook**: Maps prices to BookLevels using std::map (buy: descending, sell: ascending)
- **Pool<T>**: Pre-allocated object pool for zero-allocation order management
- **RingBuffer<T>**: Lock-free SPSC circular buffer for event streaming

## Performance Characteristics

### Complexity Analysis

| Operation | Average Case | Worst Case | Notes |
|-----------|--------------|------------|-------|
| Add Order | O(log N + M) | O(log N + M) | N = price levels, M = matches |
| Cancel Order | O(log N + K) | O(log N + K) | K = orders at price level |
| Replace Order | O(log N + K + M) | O(log N + K + M) | Cancel + Add |
| Best Bid/Ask | O(1) | O(1) | Direct map access |
| Match | O(M) | O(M) | Linear in matches |

### Benchmark Results

On modern hardware (example: Intel i7-10700K @ 3.8GHz):

```
Benchmarking with 10,000 orders...
  Average submit time: 350 ns
  Average cancel time: 280 ns
  Average replace time: 520 ns
  Throughput: ~28,000 ops/ms
```

## Building

### Requirements

- C++20 compatible compiler (GCC 10+, Clang 12+, MSVC 2019+)
- CMake 3.15+
- Python 3.8+ (for bindings)
- pybind11 (for Python bindings)
- GoogleTest (automatically fetched if not found)

### Build Instructions

```bash
# Clone the repository
git clone https://github.com/ambicuity/High-Performance-Limit-Order-Book-LOB-Simulator.git
cd High-Performance-Limit-Order-Book-LOB-Simulator

# Create build directory
mkdir build && cd build

# Configure with CMake
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build . -j$(nproc)

# Run tests
ctest --output-on-failure

# Run benchmarks
./bench_match_engine --quick
```

### Python Installation

```bash
# After building, install the Python module
cd build
cp lobsim*.so ../bindings/python/  # Or lobsim*.dylib on macOS

# Or use pip in development mode
cd ..
pip install -e bindings/python/
```

## Quick Start

### C++ API

```cpp
#include "lob/MatchingEngine.h"

using namespace lob;

int main() {
    // Configure engine
    EngineConfig config;
    config.max_orders = 100000;
    config.ring_size = 10000;
    config.tick_size = 0.01;
    
    // Create engine with simulated time
    auto time_source = std::make_shared<SimulatedTimeSource>(0);
    MatchingEngine engine(config, time_source);
    
    // Submit orders
    Order buy(1, Side::Buy, Price::from_double(100.0, 0.01), 10, 
              time_source->now_ns(), OrderType::Limit);
    Order sell(2, Side::Sell, Price::from_double(100.5, 0.01), 10, 
               time_source->now_ns(), OrderType::Limit);
    
    engine.submit(buy);
    engine.submit(sell);
    
    // Get best bid/ask
    BookTop top;
    engine.best_bid_ask(top);
    std::cout << "Best bid: " << top.best_bid.to_double(0.01) 
              << " x " << top.bid_qty << std::endl;
    
    // Poll events
    std::vector<EngineEvent> events;
    engine.poll_events(events);
    
    return 0;
}
```

### Python API

```python
from lobsim import (
    EngineConfig, MatchingEngine, Order, Price, Side, OrderType,
    SimulatedTimeSource
)

# Configure engine
config = EngineConfig(max_orders=100000, ring_size=10000, tick_size=0.01)
time_source = SimulatedTimeSource(0)
engine = MatchingEngine(config, time_source)

# Submit orders
buy = Order(1, Side.Buy, Price.from_double(100.0, 0.01), 10, 
            time_source.now_ns(), OrderType.Limit)
sell = Order(2, Side.Sell, Price.from_double(100.5, 0.01), 10, 
             time_source.now_ns(), OrderType.Limit)

engine.submit(buy)
engine.submit(sell)

# Get best bid/ask
top = engine.best_bid_ask()
print(f"Best bid: {top.best_bid.to_double(0.01)} x {top.bid_qty}")

# Poll events
events = engine.poll_events()
for event in events:
    print(f"Event: {type(event).__name__}")
```

See [examples/quickstart.py](bindings/python/examples/quickstart.py) for a complete example.

## Examples

### Market Maker Demo

The market maker demo (`bindings/python/examples/market_maker_demo.py`) demonstrates:

- Bi-directional quoting with spread management
- Inventory tracking and risk management
- Dynamic price skewing based on position
- PnL calculation

Run with:
```bash
cd bindings/python/examples
python market_maker_demo.py
```

### Depth Snapshot Demo

View multi-level order book depth (`bindings/python/examples/depth_snapshot_demo.py`):

```bash
cd bindings/python/examples
python depth_snapshot_demo.py
```

### Multi-Symbol Trading

Trade multiple symbols simultaneously (`bindings/python/examples/multi_symbol_demo.py`):

```bash
cd bindings/python/examples
python multi_symbol_demo.py
```

### Market Data Replay

Replay historical order flow from CSV (`bindings/python/examples/market_data_replay_demo.py`):

```bash
cd bindings/python/examples
python market_data_replay_demo.py
```

### Live Visualization

Open `docs/visualization.html` in a web browser to see real-time order book visualization via WebSocket feed.

### Order Flow Generation

Generate synthetic order flow for testing:

```bash
cd tools
python generate_order_flow.py -n 100000 -o orders.csv --market-pct 5 --cancel-pct 20
```

## Determinism Guarantees

The simulator is fully deterministic when using `SimulatedTimeSource`:

1. **Seeded Random Number Generation**: All stochasticity must use seeded PRNGs
2. **Injectable Time**: `TimeSource` interface allows controlled time progression
3. **Fixed Order Processing**: Same input order → same matching outcome
4. **No Hidden State**: All state changes are explicit and reproducible

This enables:
- Reproducible backtests
- Regression testing
- Strategy debugging
- Performance comparison

## Order Types

### Limit Order
Standard resting order at a specific price. Partial fills allowed.

### Market Order
Executes immediately at best available prices. Sweeps multiple levels if needed.

### IOC (Immediate-Or-Cancel)
Fills available quantity immediately, cancels remainder. Never rests on book.

### FOK (Fill-Or-Kill)
Executes in full or rejects entirely. All-or-nothing semantics.

### Iceberg Order
Order with hidden quantity. Only displays a portion of the total quantity to the market.
- `display_qty`: Visible quantity shown in order book
- `refresh_qty`: Quantity to refresh when display portion is filled
- Useful for large orders to avoid market impact

### Pegged Order
Order price automatically adjusts based on market conditions:
- **Mid Peg**: Pegged to mid-price between best bid and ask
- **Best Bid Peg**: Follows the best bid price
- **Best Ask Peg**: Follows the best ask price
- `offset`: Fixed offset in ticks from the peg reference

## Event Types

- **TradeEvent**: Order match with price, quantity, maker/taker IDs
- **AcceptEvent**: Order successfully added to book
- **RejectEvent**: Order rejected (duplicate ID, FOK not filled, etc.)
- **CancelEvent**: Order canceled with remaining quantity
- **ReplaceEvent**: Order modified (price/quantity changed)
- **BookTop**: Best bid/ask snapshot

## Limitations & Roadmap

### Current Limitations

- No stop orders or conditional orders
- No order modification without losing time priority
- Limited to FIFO matching (no pro-rata)
- WebSocket feed is a simplified implementation (requires external library for production)
- Pegged orders require manual repricing (auto-repricing not yet implemented)

### Completed Enhancements

- [x] Iceberg/hidden orders
- [x] Pegged orders (to mid, best bid/ask)
- [x] Multi-symbol support with sharding
- [x] Order book snapshots at depth
- [x] Market data replay from real exchanges
- [x] WebSocket feed for live visualization
- [x] FPGA acceleration research

### Future Work

- [ ] Automatic pegged order repricing on market updates
- [ ] Stop orders and stop-limit orders
- [ ] Pro-rata matching algorithm option
- [ ] Full WebSocket server implementation with authentication
- [ ] Historical data connectors for major exchanges
- [ ] FPGA proof-of-concept implementation

## Testing

### C++ Unit Tests

```bash
cd build
./test_book_basic
./test_engine_fills
```

### Python Tests

```bash
cd tests/python
python test_api.py
```

### Coverage

The test suite covers:
- Empty book operations
- Single-sided markets
- Crossing orders and matching
- Partial fills and remainders
- FIFO priority verification
- IOC/FOK edge cases
- Cancel/replace operations
- Determinism verification

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines.

### Code Style

- C++: Modern C++20, `clang-format` with Google style
- Python: PEP 8 with `black` formatter
- Comments: Explain "why", not "what"
- Tests: Required for all new features

## License

MIT License - see [LICENSE](LICENSE) for details.

## References

- [Matching Engine Design](https://web.archive.org/web/20110219163448/http://howtohft.wordpress.com/2011/02/15/how-to-build-a-fast-limit-order-book/)
- [Price-Time Priority Matching](https://www.cmegroup.com/confluence/display/EPICSANDBOX/Matching+Algorithms)
- pybind11: https://github.com/pybind/pybind11

## Contact

For questions or collaboration: Ritesh Rana

---

**Built with ❤️ for high-frequency trading research and education.**