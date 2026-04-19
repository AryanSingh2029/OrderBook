# High-Performance Order Book and Matching Engine

A multi-instrument `C++20` limit order book and matching engine built to explore the kinds of systems design, correctness, and latency questions that matter in high-frequency trading.

This project implements a central limit order book with price-time priority, deterministic event replay, queue-position tracking, microstructure analytics, benchmarking, logging, and a lightweight replay UI for understanding how the market evolves event by event.

## Why This Project Exists

Most order book projects stop at "orders match." This one goes further and tries to answer:

- Is the matching logic correct?
- Is the system deterministic under replay?
- Can we inspect and debug every state transition?
- Can we measure latency, throughput, and memory behavior?
- Can we expose market microstructure concepts like imbalance, queue position, and slippage?
- Can we support multiple instruments cleanly without turning the design into a mess?

The goal is to build something that is both:
- technically strong enough to discuss in HFT / low-latency / systems interviews
- understandable enough to use as a learning tool

## Core Features

### Matching Engine

- Central limit order book
- Two-sided market: `BID` / `ASK`
- Price-time priority
- Limit orders
- Market orders
- Cancel orders
- Replace / modify orders
- Partial fills
- Resting unmatched limit orders
- Non-resting market orders

### Order Book Design

- Fast `order_id -> order location` lookup
- Slot-pool based order storage to reduce allocation overhead
- Cached price-level aggregates
- FIFO queue behavior within each price level
- Queue position tracking:
  - orders ahead
  - quantity ahead
  - level order count
  - level total quantity

### Exchange / Market Structure

- Multiple instruments supported
- Separate matching engine per symbol
- Symbol-aware routing via exchange layer
- Deterministic replay across symbols

### Replay / Simulation

- Event-driven simulator
- CSV-based event ingestion
- Replay of timestamped market events
- Structured replay results per event
- Deterministic final state for identical input streams

### Analytics / Microstructure

- Best bid / best ask
- Spread
- Mid price
- Top-N depth
- Total bid / ask volume
- Top-of-book imbalance
- Depth imbalance
- Traded volume
- Execution count
- VWAP
- Average trade size
- Fill ratio
- Slippage
- Queue depletion

### Reporting / Debugging

- Trade log output
- Rejection log output
- Book snapshots
- Order state logs
- Metrics logs
- End-of-run summary
- Final top-5 levels view
- Debug replay mode vs summary mode

### Benchmarking

- Small, medium, and heavy-load scenarios
- Throughput measurement
- Average latency
- Worst-case latency
- `p50` / `p99` latency
- Peak memory usage
- Latency histograms
- JSON benchmark output for visualization

### Strategy Sandbox

- Simple imbalance-based taker strategy
- Tracks:
  - inventory
  - realized PnL
  - unrealized PnL
  - total PnL
  - fill ratio
  - adverse selection

### UI

- Local replay UI for visual understanding
- Event-by-event stepping
- Symbol selection
- Top-of-book and depth view
- Trade view
- Event diff panel
- Queue position panel
- Rejections panel
- Microstructure timeline chart
- Depth heatmap
- Strategy stats panel
- Benchmark dashboard

## Architecture

The project is intentionally layered so each component has a clear responsibility.

### 1. `Order`

Defines the basic market objects:

- order id
- side
- order type
- price
- quantity
- remaining quantity
- timestamp

### 2. `OrderBook`

Owns the market state for a single instrument:

- bid side
- ask side
- price levels
- resting orders
- order lookup
- queue position

This is where the core matching and resting logic lives.

### 3. `MatchingEngine`

Wraps the order book with higher-level order lifecycle tracking:

- `ACTIVE`
- `FILLED`
- `CANCELLED`
- `REPLACED`
- `REJECTED`

It returns structured per-event results and state updates.

### 4. `Exchange`

Routes events by symbol and owns one matching engine per instrument.

This is what enables:

- `AAPL`
- `MSFT`
- `BTCUSD`

to have completely separate books.

### 5. `Simulator`

Loads events and replays them in timestamp order through the exchange.

This makes the system deterministic and event-driven.

### 6. `Analytics`

Computes market and execution metrics from:

- book state
- replay state
- trade results

### 7. `Reporting`

Writes replay artifacts to disk:

- `trades.csv`
- `snapshots.csv`
- `rejections.csv`
- `states.csv`
- `metrics.csv`

### 8. `Strategy`

Runs a small trading sandbox on top of the replayed market.

### 9. `UI`

Reads replay artifacts and visualizes them without touching engine internals.

## Matching Rules

The engine uses price-time priority.

### Buy Order Matching

An incoming buy order matches the lowest available ask first.

### Sell Order Matching

An incoming sell order matches the highest available bid first.

### Priority Rules

1. Better price wins first
2. At the same price, earlier order wins first

### Limit Order Behavior

A limit order:

- trades immediately if it crosses the opposite side
- otherwise rests in the book
- may partially match and then rest with the remaining quantity

### Market Order Behavior

A market order:

- matches immediately against the opposite side
- continues until filled or book is empty
- never rests in the book

### Cancel Behavior

A cancel removes a live resting order if it exists.
If the order is already gone or never existed, the cancel fails safely.

### Replace / Modify Behavior

Price-changing replaces are treated as remove + reinsert, which gives the order fresh time priority.

## Event Format

The simulator consumes CSV events of the form:

```csv
timestamp,symbol,type,side,order_id,price,quantity
1,AAPL,ADD,BUY,1101,100,100
2,AAPL,ADD,SELL,1102,101,50
3,AAPL,MARKET,BUY,1103,,30
4,AAPL,CANCEL,,1101,,
```

### Event Types

- `ADD` = limit order
- `MARKET` = market order
- `CANCEL` = cancel request

### Interpretation

- `ADD` creates a limit order that may match or rest
- `MARKET` creates a market order that executes immediately and never rests
- `CANCEL` removes an existing resting order by `order_id`

## Project Structure

```text
orderbook/
├── src/
│   ├── analytics.cpp
│   ├── exchange.cpp
│   ├── main.cpp
│   ├── matching_engine.cpp
│   ├── order.cpp
│   ├── order_book.cpp
│   ├── reporting.cpp
│   ├── simulator.cpp
│   ├── strategy.cpp
│   ├── strategy_main.cpp
│   └── utils.cpp
│
├── include/
│   ├── analytics.h
│   ├── exchange.h
│   ├── matching_engine.h
│   ├── order.h
│   ├── order_book.h
│   ├── reporting.h
│   ├── simulator.h
│   ├── strategy.h
│   └── utils.h
│
├── tests/
│   ├── test_analytics.cpp
│   ├── test_basic.cpp
│   ├── test_cancel.cpp
│   ├── test_matching.cpp
│   ├── test_partial_fill.cpp
│   ├── test_simulator.cpp
│   └── test_strategy.cpp
│
├── data/
│   ├── sample_events.csv
│   ├── benchmark_events.csv
│   └── sample_commands.txt
│
├── benchmarks/
│   └── benchmark.cpp
│
├── docs/
│   ├── design.md
│   ├── matching_rules.md
│   └── benchmark_report.md
│
├── ui/
│   ├── index.html
│   ├── app.js
│   ├── styles.css
│   └── README.md
│
├── logs/
├── README.md
├── Makefile
└── CMakeLists.txt
```

## Build and Run

### Build Everything

```bash
make
```

### Run Main Replay Demo

```bash
make run
```

This:

- builds the replay executable
- replays `data/sample_events.csv`
- prints event-by-event debug output
- writes logs to `logs/`

### Run Tests

```bash
make test
```

### Run Benchmark

```bash
make bench
```

### Run Strategy Sandbox

```bash
./build/strategy_demo
```

### Clean Build Artifacts

```bash
make clean
```

## Understanding the Outputs

### Terminal Replay Output

`make run` prints:

- event being processed
- trades generated
- order state changes
- execution metrics
- top of book
- visible bid / ask levels
- market metrics
- final run summary

### Generated Log Files

After replay, the engine writes:

- `logs/trades.csv`
  - every trade generated by replay
- `logs/snapshots.csv`
  - visible price levels after each event
- `logs/rejections.csv`
  - invalid or rejected events
- `logs/states.csv`
  - order lifecycle and queue-position data
- `logs/metrics.csv`
  - market and execution metrics by event

### Strategy Output

After strategy run:

- `logs/strategy_summary.json`

### Benchmark Output

After benchmark run:

- `logs/benchmark_summary.json`

## Testing Philosophy

This project emphasizes correctness before performance claims.

### What Is Tested

- basic order insertion
- invalid input rejection
- full fills
- partial fills
- multiple matches across price levels
- FIFO behavior at same price
- cancel behavior
- replace behavior
- deterministic replay
- multi-instrument separation
- analytics correctness
- strategy summary behavior

### Why This Matters

A matching engine that is fast but wrong is not useful.
The tests are there to ensure:

- matching rules are consistent
- edge cases are handled explicitly
- replay is reproducible
- metrics are trustworthy

## Performance and Latency Work

The project includes a benchmark harness that measures:

- total events processed
- throughput in events/sec
- average latency
- worst-case latency
- `p50` latency
- `p99` latency
- peak memory usage
- latency histogram distribution

### Current Design Choices That Improve Performance

- slot-pool based order storage instead of per-order linked-list heap churn
- cached price-level aggregates
- fast order lookup by `order_id`
- explicit separation between hot-path matching and analytics/reporting

### Why the Numbers Matter

The goal is not to claim exchange-grade production performance.
The value is in:

- measuring the system
- comparing before/after optimizations
- explaining tradeoffs clearly

## Queue Position Tracking

One of the stronger HFT-style features in this project is queue-position tracking.

For active resting orders, the system can compute:

- number of orders ahead
- quantity ahead
- total orders at the level
- total quantity at the level

This is important because in price-time-priority markets, two orders at the same price are not equal: the earlier one gets executed first.

Queue position makes the project more realistic than a simple aggregate-size-only book.

## Microstructure Metrics

This project is not just a data-structure exercise. It also computes metrics traders and researchers actually care about.

### Top-of-Book Metrics

- best bid
- best ask
- spread
- mid

### Depth Metrics

- top-N bid volume
- top-N ask volume
- cumulative visible depth

### Imbalance Metrics

- top-of-book imbalance
- depth imbalance
- total bid vs ask volume

### Trade Metrics

- traded volume
- execution count
- VWAP
- average trade size

### Execution Metrics

- fill ratio
- slippage
- queue depletion

## Replay UI

A lightweight local web UI is included to make the system easier to understand visually.

### What the UI Shows

- current event
- symbol selector
- top of book
- bid/ask depth
- trades at current event
- recent trades
- event diff
- queue-position panel
- rejection panel
- spread/mid/imbalance timeline
- depth heatmap
- strategy summary
- benchmark dashboard

### Start the UI

First generate the data:

```bash
make run
./build/strategy_demo
make bench
```

Then start a local server:

```bash
python3 -m http.server 8000
```

Open:

```text
http://localhost:8000/ui/
```

## Example Walkthrough

Given an event stream like:

```csv
1,AAPL,ADD,BUY,1101,100,100
2,AAPL,ADD,SELL,1102,102,60
3,AAPL,ADD,SELL,1103,100,40
```

### What Happens

- Event 1: buy limit order rests at `100`
- Event 2: sell limit order rests at `102`
- Event 3: sell limit order at `100` crosses the resting buy at `100`
- A trade occurs
- The sell may fill completely
- The buy may remain partially active depending on quantity

This is the core flow of the engine: event comes in -> engine decides whether to match or rest -> book updates -> logs/metrics/UI update.

## Design Principles

### 1. Correctness First

Matching rules and edge cases are explicit and tested.

### 2. Separation of Concerns

The book, engine, replay, analytics, reporting, and UI are separate modules.

### 3. Determinism

Same event stream should always produce the same result.

### 4. Performance Awareness

The implementation is designed with latency and allocation behavior in mind.

### 5. Observability

Everything important can be logged, inspected, benchmarked, or visualized.

## What This Project Demonstrates

This project demonstrates experience with:

- modern `C++`
- low-latency systems thinking
- event-driven architecture
- performance measurement
- deterministic simulation
- exchange-style matching rules
- order lifecycle tracking
- market microstructure analytics
- testing and debugging discipline
