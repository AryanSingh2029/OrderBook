# Benchmark Report

This file records example benchmark output from the current `C++20` order book implementation.

## Metrics captured

- total events processed
- total runtime
- throughput in events per second
- average latency per event
- worst-case latency
- p50 and p99 event latency
- latency histograms by bucket
- peak RSS memory footprint

## Run command

```bash
make bench
```

## Latest local run

### Baseline run

### Small correctness run

- `500` events processed in `0.272 ms`
- throughput: `1,835,138 events/s`
- average latency: `510 ns`
- worst-case latency: `18,875 ns`
- `p50`: `333 ns`
- `p99`: `2,667 ns`
- peak memory: `1.625 MB`

### Medium load

- `50,000` events processed in `18.292 ms`
- throughput: `2,733,391 events/s`
- average latency: `334 ns`
- worst-case latency: `120,959 ns`
- `p50`: `250 ns`
- `p99`: `1,250 ns`
- peak memory: `9.844 MB`

### Heavy load

- `500,000` events processed in `96.462 ms`
- throughput: `5,183,406 events/s`
- average latency: `177 ns`
- worst-case latency: `902,084 ns`
- `p50`: `125 ns`
- `p99`: `666 ns`
- peak memory: `87.281 MB`

## Optimized local run

This run reflects the lower-allocation order book design that replaced per-order `std::list` nodes with an intrusive slot pool and cached per-level aggregates.

### Small correctness run

- `500` events processed in `0.318 ms`
- throughput: `1,571,091 events/s`
- average latency: `597 ns`
- worst-case latency: `23,875 ns`
- `p50`: `334 ns`
- `p99`: `2,875 ns`
- peak memory: `1.609 MB`

### Medium load

- `50,000` events processed in `15.451 ms`
- throughput: `3,235,975 events/s`
- average latency: `280 ns`
- worst-case latency: `88,584 ns`
- `p50`: `208 ns`
- `p99`: `1,083 ns`
- peak memory: `9.891 MB`

### Heavy load

- `500,000` events processed in `86.885 ms`
- throughput: `5,754,733 events/s`
- average latency: `158 ns`
- worst-case latency: `1,097,375 ns`
- `p50`: `125 ns`
- `p99`: `625 ns`
- peak memory: `89.188 MB`

## Histogram-enabled local run

This run keeps the optimized order book design and adds fixed latency buckets so the benchmark output shows the distribution shape instead of only point summaries.

### Small correctness run

- `500` events processed in `0.326 ms`
- throughput: `1,534,919 events/s`
- average latency: `626 ns`
- worst-case latency: `43,500 ns`
- `p50`: `333 ns`
- `p99`: `4,583 ns`
- peak memory: `1.656 MB`
- histogram:
  - `<=100ns`: `41`
  - `101-250ns`: `157`
  - `251-500ns`: `143`
  - `501ns-1us`: `110`
  - `1-2us`: `32`
  - `2-5us`: `13`
  - `5-10us`: `3`
  - `>10us`: `1`

### Medium load

- `50,000` events processed in `15.370 ms`
- throughput: `3,253,134 events/s`
- average latency: `279 ns`
- worst-case latency: `103,500 ns`
- `p50`: `208 ns`
- `p99`: `1,125 ns`
- peak memory: `11.406 MB`
- histogram:
  - `<=100ns`: `7,736`
  - `101-250ns`: `26,526`
  - `251-500ns`: `9,663`
  - `501ns-1us`: `5,386`
  - `1-2us`: `601`
  - `2-5us`: `56`
  - `5-10us`: `16`
  - `>10us`: `16`

### Heavy load

- `500,000` events processed in `89.976 ms`
- throughput: `5,557,042 events/s`
- average latency: `164 ns`
- worst-case latency: `982,667 ns`
- `p50`: `125 ns`
- `p99`: `584 ns`
- peak memory: `101.453 MB`
- histogram:
  - `<=100ns`: `226,294`
  - `101-250ns`: `195,523`
  - `251-500ns`: `69,453`
  - `501ns-1us`: `7,596`
  - `1-2us`: `932`
  - `2-5us`: `78`
  - `5-10us`: `98`
  - `>10us`: `26`

## Comparison notes

- The optimized design improves `medium-load` throughput from `2.73M` to `3.24M events/s`.
- The optimized design improves `heavy-load` throughput from `5.18M` to `5.75M events/s`.
- Average latency improved from `334 ns` to `280 ns` on medium load and from `177 ns` to `158 ns` on heavy load.
- Peak memory is slightly higher in the optimized run because the slot-pool keeps reusable order storage around instead of freeing each node immediately.
- Small-run latency regressed slightly, which is common when a design is optimized for sustained load rather than tiny runs.
- The histogram-enabled run shows that the heavy-load latency mass is concentrated in the `<=250ns` buckets, with only a very small long tail beyond `1us`.

## Notes

- Benchmarks use deterministic synthetic event generation with a fixed RNG seed.
- Three scenarios are exercised: small correctness-sized load, medium load, and heavy load.
- Peak memory uses process RSS from `getrusage`, so it is a practical runtime estimate rather than a custom allocator-level accounting.
