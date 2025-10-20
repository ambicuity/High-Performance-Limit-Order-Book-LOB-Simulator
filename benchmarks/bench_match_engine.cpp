#include "lob/MatchingEngine.h"
#include "lob/TimeSource.h"
#include <iostream>
#include <chrono>
#include <random>
#include <iomanip>

using namespace lob;

struct BenchmarkResults {
    double avg_submit_ns;
    double avg_cancel_ns;
    double avg_replace_ns;
    size_t total_trades;
    size_t total_operations;
};

BenchmarkResults run_benchmark(size_t num_orders, bool quick_mode = false) {
    EngineConfig config;
    config.max_orders = num_orders * 2;
    config.ring_size = num_orders * 10;
    config.tick_size = 0.01;
    
    auto time_source = std::make_shared<SimulatedTimeSource>(1000000000);
    MatchingEngine engine(config, time_source);
    
    std::mt19937_64 rng(12345); // Fixed seed for reproducibility
    std::uniform_real_distribution<double> price_dist(99.0, 101.0);
    std::uniform_int_distribution<uint64_t> qty_dist(1, 100);
    std::uniform_int_distribution<int> side_dist(0, 1);
    
    BenchmarkResults results{};
    
    std::vector<OrderId> active_orders;
    active_orders.reserve(num_orders);
    
    // Benchmark order submission
    auto start = std::chrono::high_resolution_clock::now();
    
    for (size_t i = 0; i < num_orders; i++) {
        OrderId id = i + 1;
        Side side = side_dist(rng) == 0 ? Side::Buy : Side::Sell;
        double price = price_dist(rng);
        uint64_t qty = qty_dist(rng);
        
        Order order(id, side, Price::from_double(price, 0.01), qty, 
                   time_source->now_ns(), OrderType::Limit);
        
        if (engine.submit(order)) {
            active_orders.push_back(id);
        }
        
        time_source->advance(100); // 100ns per order
        
        if (quick_mode && i >= num_orders / 10) {
            break; // Quick mode only processes 10%
        }
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
    
    size_t actual_orders = quick_mode ? num_orders / 10 : num_orders;
    results.avg_submit_ns = static_cast<double>(duration.count()) / actual_orders;
    results.total_operations = actual_orders;
    
    // Poll events
    std::vector<EngineEvent> events;
    engine.poll_events(events);
    
    // Count trades
    for (const auto& event : events) {
        if (std::holds_alternative<TradeEvent>(event)) {
            results.total_trades++;
        }
    }
    
    // Benchmark cancels
    if (!active_orders.empty() && !quick_mode) {
        size_t cancel_count = std::min(active_orders.size() / 4, size_t(1000));
        start = std::chrono::high_resolution_clock::now();
        
        for (size_t i = 0; i < cancel_count; i++) {
            size_t idx = i % active_orders.size();
            engine.cancel(active_orders[idx]);
            time_source->advance(50);
        }
        
        end = std::chrono::high_resolution_clock::now();
        duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
        results.avg_cancel_ns = static_cast<double>(duration.count()) / cancel_count;
        results.total_operations += cancel_count;
        
        // Benchmark replaces
        size_t replace_count = std::min(active_orders.size() / 4, size_t(1000));
        start = std::chrono::high_resolution_clock::now();
        
        for (size_t i = 0; i < replace_count; i++) {
            size_t idx = (cancel_count + i) % active_orders.size();
            double new_price = price_dist(rng);
            uint64_t new_qty = qty_dist(rng);
            engine.replace(active_orders[idx], Price::from_double(new_price, 0.01), new_qty);
            time_source->advance(50);
        }
        
        end = std::chrono::high_resolution_clock::now();
        duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
        results.avg_replace_ns = static_cast<double>(duration.count()) / replace_count;
        results.total_operations += replace_count;
    } else {
        results.avg_cancel_ns = 0;
        results.avg_replace_ns = 0;
    }
    
    return results;
}

int main(int argc, char** argv) {
    bool quick_mode = false;
    
    // Check for --quick flag
    for (int i = 1; i < argc; i++) {
        if (std::string(argv[i]) == "--quick") {
            quick_mode = true;
        }
    }
    
    std::cout << "=== High-Performance LOB Simulator Benchmark ===" << std::endl;
    std::cout << "Mode: " << (quick_mode ? "Quick" : "Full") << std::endl << std::endl;
    
    std::vector<size_t> test_sizes = quick_mode ? 
        std::vector<size_t>{1000, 10000} : 
        std::vector<size_t>{1000, 10000, 100000};
    
    for (size_t num_orders : test_sizes) {
        std::cout << "Benchmarking with " << num_orders << " orders..." << std::endl;
        
        auto results = run_benchmark(num_orders, quick_mode);
        
        std::cout << std::fixed << std::setprecision(2);
        std::cout << "  Average submit time: " << results.avg_submit_ns << " ns" << std::endl;
        
        if (results.avg_cancel_ns > 0) {
            std::cout << "  Average cancel time: " << results.avg_cancel_ns << " ns" << std::endl;
        }
        
        if (results.avg_replace_ns > 0) {
            std::cout << "  Average replace time: " << results.avg_replace_ns << " ns" << std::endl;
        }
        
        std::cout << "  Total trades generated: " << results.total_trades << std::endl;
        std::cout << "  Total operations: " << results.total_operations << std::endl;
        
        double ops_per_ms = results.total_operations / (results.avg_submit_ns * results.total_operations / 1000000.0);
        std::cout << "  Throughput: " << ops_per_ms << " ops/ms" << std::endl;
        std::cout << std::endl;
    }
    
    std::cout << "Benchmark complete!" << std::endl;
    
    return 0;
}
