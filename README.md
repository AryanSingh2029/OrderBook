# Order Book Project

This repository contains a single-instrument central limit order book and matching engine implemented in `C++20`. The project is structured to grow from a clean interview-ready foundation into a more serious HFT systems project with replay, benchmarking, and lower-latency internal designs.

## Stage 0 and Stage 1 status

The repository now includes:

- a written design doc in `docs/design.md`
- explicit matching rules in `docs/matching_rules.md`
- a modular `C++` codebase split across engine, book, simulator, and utilities
- unit tests split by behavior
- sample CSV event files
- a benchmark harness
- both `Makefile` and `CMakeLists.txt` project setup

## Current features

- single instrument limit order book
- bid and ask sides
- price-time priority
- limit orders
- market orders
- cancel orders
- replace or modify support
- partial fills
- resting unmatched limit orders
- non-resting market orders
- deterministic event-stream replay from CSV files
- command stream replay from human-readable input
- explicit trade records and order state transitions
- market microstructure analytics for top-of-book, depth, imbalance, trade, and execution quality
- scenario-based benchmarking for throughput, latency, and peak memory footprint
- latency histograms for benchmark distribution analysis
- replay reporting with trade logs, rejection logs, snapshots, and end-of-run summaries
- optional strategy sandbox with a toy imbalance-based taker strategy
- simple local web replay UI for visualizing events, book state, and trades

## Project layout

- `src/`: engine implementation and demo entrypoint
- `include/`: public headers
- `tests/`: focused correctness tests
- `data/`: sample event streams
- `docs/`: design and rules
- `benchmarks/`: throughput and latency benchmark

## Build and run

`cmake` is declared for repo structure and portability, but this environment does not currently have it installed. The verified local path is the `Makefile`.

```bash
make
make run
make test
make bench
```

The demo executable now reads [`data/sample_events.csv`](/Users/aryansingh/OrderBook/data/sample_events.csv) by default, or a custom event file passed as an argument. Event streams now support multiple instruments via a `symbol` column:

```bash
./build/demo data/sample_events.csv
```

Debug replay mode writes:
- `logs/trades.csv`
- `logs/rejections.csv`
- `logs/snapshots.csv`

You can switch between detailed replay output and summary-only replay:

```bash
./build/demo --mode=debug data/sample_events.csv
./build/demo --mode=summary data/sample_events.csv
```

The strategy sandbox is intentionally lightweight and separate from the core engine:

```bash
./build/strategy_demo data/sample_events.csv
```

For a visual walkthrough of replay output:

```bash
python3 -m http.server 8000
```

Then open `http://localhost:8000/ui/` after running:

```bash
make run
```

## Resume framing

One concise way to present the project:

> Built a `C++20` central limit order book and matching engine with price-time priority, deterministic event replay, market microstructure analytics, efficient cancel lookup, and scenario-based latency/throughput benchmarking for trading-style workloads.

## Strong next steps

The best upgrades from here are:

1. add a custom memory pool to reduce allocation overhead
2. support full replay from larger event datasets
3. extend the simulator with richer exchange-style messages
4. compare alternative data structures and measure latency tradeoffs
