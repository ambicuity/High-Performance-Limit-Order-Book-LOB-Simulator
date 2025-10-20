#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/operators.h>
#include "lob/Config.h"
#include "lob/MatchingEngine.h"
#include "lob/Order.h"
#include "lob/Events.h"
#include "lob/TimeSource.h"
#include "lob/MarketDataFeed.h"

namespace py = pybind11;

PYBIND11_MODULE(lobsim, m) {
    m.doc() = "High-Performance Limit Order Book Simulator";

    // Enums
    py::enum_<lob::Side>(m, "Side")
        .value("Buy", lob::Side::Buy)
        .value("Sell", lob::Side::Sell)
        .export_values();

    py::enum_<lob::OrderType>(m, "OrderType")
        .value("Limit", lob::OrderType::Limit)
        .value("Market", lob::OrderType::Market)
        .value("IOC", lob::OrderType::IOC)
        .value("FOK", lob::OrderType::FOK)
        .export_values();

    py::enum_<lob::EventType>(m, "EventType")
        .value("Trade", lob::EventType::Trade)
        .value("OrderAccepted", lob::EventType::OrderAccepted)
        .value("OrderRejected", lob::EventType::OrderRejected)
        .value("OrderCanceled", lob::EventType::OrderCanceled)
        .value("OrderReplaced", lob::EventType::OrderReplaced)
        .value("BookUpdate", lob::EventType::BookUpdate)
        .export_values();

    // Price
    py::class_<lob::Price>(m, "Price")
        .def(py::init<>())
        .def(py::init<int64_t>())
        .def_readwrite("ticks", &lob::Price::ticks)
        .def_static("from_double", &lob::Price::from_double, 
                    py::arg("price"), py::arg("tick_size"))
        .def("to_double", &lob::Price::to_double, py::arg("tick_size"))
        .def(py::self == py::self)
        .def(py::self != py::self)
        .def(py::self < py::self)
        .def(py::self <= py::self)
        .def(py::self > py::self)
        .def(py::self >= py::self)
        .def("__repr__", [](const lob::Price& p) {
            return "Price(ticks=" + std::to_string(p.ticks) + ")";
        });

    // Order
    py::class_<lob::Order>(m, "Order")
        .def(py::init<>())
        .def(py::init<lob::OrderId, lob::Side, lob::Price, uint64_t, uint64_t, lob::OrderType>(),
             py::arg("id"), py::arg("side"), py::arg("price"), 
             py::arg("qty"), py::arg("ts"), py::arg("type") = lob::OrderType::Limit)
        .def_readwrite("id", &lob::Order::id)
        .def_readwrite("side", &lob::Order::side)
        .def_readwrite("price", &lob::Order::price)
        .def_readwrite("qty", &lob::Order::qty)
        .def_readwrite("ts", &lob::Order::ts)
        .def_readwrite("type", &lob::Order::type)
        .def("is_market", &lob::Order::is_market)
        .def("is_limit", &lob::Order::is_limit)
        .def("is_ioc", &lob::Order::is_ioc)
        .def("is_fok", &lob::Order::is_fok);

    // Events
    py::class_<lob::TradeEvent>(m, "TradeEvent")
        .def(py::init<>())
        .def_readwrite("type", &lob::TradeEvent::type)
        .def_readwrite("taker_id", &lob::TradeEvent::taker_id)
        .def_readwrite("maker_id", &lob::TradeEvent::maker_id)
        .def_readwrite("price", &lob::TradeEvent::price)
        .def_readwrite("qty", &lob::TradeEvent::qty)
        .def_readwrite("ts", &lob::TradeEvent::ts);

    py::class_<lob::AcceptEvent>(m, "AcceptEvent")
        .def(py::init<>())
        .def_readwrite("type", &lob::AcceptEvent::type)
        .def_readwrite("id", &lob::AcceptEvent::id)
        .def_readwrite("ts", &lob::AcceptEvent::ts);

    py::class_<lob::RejectEvent>(m, "RejectEvent")
        .def(py::init<>())
        .def_readwrite("type", &lob::RejectEvent::type)
        .def_readwrite("id", &lob::RejectEvent::id)
        .def_readwrite("ts", &lob::RejectEvent::ts)
        .def_readwrite("reason_code", &lob::RejectEvent::reason_code);

    py::class_<lob::CancelEvent>(m, "CancelEvent")
        .def(py::init<>())
        .def_readwrite("type", &lob::CancelEvent::type)
        .def_readwrite("id", &lob::CancelEvent::id)
        .def_readwrite("remaining", &lob::CancelEvent::remaining)
        .def_readwrite("ts", &lob::CancelEvent::ts);

    py::class_<lob::ReplaceEvent>(m, "ReplaceEvent")
        .def(py::init<>())
        .def_readwrite("type", &lob::ReplaceEvent::type)
        .def_readwrite("id", &lob::ReplaceEvent::id)
        .def_readwrite("new_price", &lob::ReplaceEvent::new_price)
        .def_readwrite("new_qty", &lob::ReplaceEvent::new_qty)
        .def_readwrite("ts", &lob::ReplaceEvent::ts);

    py::class_<lob::BookTop>(m, "BookTop")
        .def(py::init<>())
        .def_readwrite("type", &lob::BookTop::type)
        .def_readwrite("best_bid", &lob::BookTop::best_bid)
        .def_readwrite("bid_qty", &lob::BookTop::bid_qty)
        .def_readwrite("best_ask", &lob::BookTop::best_ask)
        .def_readwrite("ask_qty", &lob::BookTop::ask_qty)
        .def_readwrite("ts", &lob::BookTop::ts);

    // Config
    py::class_<lob::EngineConfig>(m, "EngineConfig")
        .def(py::init<>())
        .def(py::init<size_t, size_t, double>(),
             py::arg("max_orders"), py::arg("ring_size"), py::arg("tick_size"))
        .def_readwrite("max_orders", &lob::EngineConfig::max_orders)
        .def_readwrite("ring_size", &lob::EngineConfig::ring_size)
        .def_readwrite("tick_size", &lob::EngineConfig::tick_size);

    // TimeSource
    py::class_<lob::TimeSource, std::shared_ptr<lob::TimeSource>>(m, "TimeSource")
        .def("now_ns", &lob::TimeSource::now_ns);

    py::class_<lob::SimulatedTimeSource, lob::TimeSource, 
               std::shared_ptr<lob::SimulatedTimeSource>>(m, "SimulatedTimeSource")
        .def(py::init<uint64_t>(), py::arg("initial_ns") = 0)
        .def("advance", &lob::SimulatedTimeSource::advance)
        .def("set", &lob::SimulatedTimeSource::set)
        .def("now_ns", &lob::SimulatedTimeSource::now_ns);

    py::class_<lob::RealTimeSource, lob::TimeSource, 
               std::shared_ptr<lob::RealTimeSource>>(m, "RealTimeSource")
        .def(py::init<>())
        .def("now_ns", &lob::RealTimeSource::now_ns);

    // MatchingEngine
    py::class_<lob::MatchingEngine>(m, "MatchingEngine")
        .def(py::init<const lob::EngineConfig&, std::shared_ptr<lob::TimeSource>>(),
             py::arg("config"), py::arg("time_source") = nullptr)
        .def("submit", &lob::MatchingEngine::submit, py::arg("order"))
        .def("cancel", &lob::MatchingEngine::cancel, py::arg("order_id"))
        .def("replace", &lob::MatchingEngine::replace,
             py::arg("order_id"), py::arg("new_price"), py::arg("new_qty"))
        .def("poll_events", [](lob::MatchingEngine& engine) {
            std::vector<lob::EngineEvent> events;
            engine.poll_events(events);
            
            py::list result;
            for (const auto& event : events) {
                if (std::holds_alternative<lob::TradeEvent>(event)) {
                    result.append(std::get<lob::TradeEvent>(event));
                } else if (std::holds_alternative<lob::AcceptEvent>(event)) {
                    result.append(std::get<lob::AcceptEvent>(event));
                } else if (std::holds_alternative<lob::RejectEvent>(event)) {
                    result.append(std::get<lob::RejectEvent>(event));
                } else if (std::holds_alternative<lob::CancelEvent>(event)) {
                    result.append(std::get<lob::CancelEvent>(event));
                } else if (std::holds_alternative<lob::ReplaceEvent>(event)) {
                    result.append(std::get<lob::ReplaceEvent>(event));
                } else if (std::holds_alternative<lob::BookTop>(event)) {
                    result.append(std::get<lob::BookTop>(event));
                }
            }
            return result;
        })
        .def("best_bid_ask", [](const lob::MatchingEngine& engine) {
            lob::BookTop top;
            engine.best_bid_ask(top);
            return top;
        })
        .def("now", &lob::MatchingEngine::now)
        .def("config", &lob::MatchingEngine::config);

    // MarketDataFeed
    py::class_<lob::MDOrder>(m, "MDOrder")
        .def(py::init<>())
        .def_readwrite("ts_ns", &lob::MDOrder::ts_ns)
        .def_readwrite("order_id", &lob::MDOrder::order_id)
        .def_readwrite("side", &lob::MDOrder::side)
        .def_readwrite("price", &lob::MDOrder::price)
        .def_readwrite("qty", &lob::MDOrder::qty)
        .def_readwrite("type", &lob::MDOrder::type)
        .def_readwrite("new_price", &lob::MDOrder::new_price)
        .def_readwrite("new_qty", &lob::MDOrder::new_qty);

    py::class_<lob::MarketDataFeed>(m, "MarketDataFeed")
        .def(py::init<>())
        .def("load_orders", &lob::MarketDataFeed::load_orders)
        .def("load_quotes", &lob::MarketDataFeed::load_quotes)
        .def("load_trades", &lob::MarketDataFeed::load_trades)
        .def_static("to_order", &lob::MarketDataFeed::to_order)
        .def_static("parse_order_type", &lob::MarketDataFeed::parse_order_type);
}
