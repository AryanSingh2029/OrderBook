# UI Notes

This is a small static replay UI for understanding the engine visually.

## Run locally

From the repository root:

```bash
python3 -m http.server 8000
```

Then open:

```text
http://localhost:8000/ui/
```

The UI reads:

- `data/sample_events.csv`
- `logs/trades.csv`
- `logs/snapshots.csv`
- `logs/rejections.csv`

If you want fresh data, run the replay first:

```bash
make run
```
----- first time 
make run
./build/strategy_demo
make bench
python3 -m http.server 8000

------------- then 
---------Only repeat these when you want to regenerate the underlying data:

make run
./build/strategy_demo
make bench
---------------What each one does:

make run -refreshes replay logs like trades, snapshots, states, metrics
./build/strategy_demo -refreshes strategy UI data
make bench- refreshes benchmark UI data
If you only want to reopen the UI, you usually just need the local server:

----python3 -m http.server 8000

## Running without ui. 
Without the UI, the main way to see the project working is through the terminal output and the generated log files.

Use this flow:

1. Run the replay engine
```bash
make run
```

That shows, event by event:
- current event
- trades generated
- order state updates
- top of book
- bid/ask levels
- market metrics
- execution metrics
- final summary

2. Read the generated logs in `logs/`
- [trades.csv](/Users/aryansingh/OrderBook/logs/trades.csv)
- [snapshots.csv](/Users/aryansingh/OrderBook/logs/snapshots.csv)
- [rejections.csv](/Users/aryansingh/OrderBook/logs/rejections.csv)
- [states.csv](/Users/aryansingh/OrderBook/logs/states.csv)
- [metrics.csv](/Users/aryansingh/OrderBook/logs/metrics.csv)

These let you inspect the system in a structured way.

3. Run tests
```bash
make test
```

This shows correctness, even if it is less visual.

4. Run the strategy sandbox
```bash
./build/strategy_demo
```

This shows:
- signals triggered
- orders sent
- fills
- inventory
- PnL
- adverse selection

5. Run benchmarks
```bash
make bench
```

This shows:
- throughput
- average latency
- p50 / p99
- worst-case latency
- memory
- latency histogram

So without UI, your project is explored through:
- `terminal replay`
- `CSV/JSON logs`
- `tests`
- `strategy output`
- `benchmark output`

The most useful command for understanding the engine itself is still:
```bash
make run
```

Because that prints the event-by-event order book evolution directly.
