benchmarks/ is the performance layer of the project.

If tests/ proves the system is correct, benchmarks/ proves how fast it is.

Folder: benchmarks

Main file:

benchmark.cpp
Think of it as:

benchmarks/ = how we measure latency, throughput, and memory

Why this folder matters
For an HFT-style project, correctness is not enough.

You also want to show:

how many events the engine can process
how long each event takes
how latency behaves under load
how memory grows under larger workloads
That is exactly what this folder is for.

What benchmark.cpp does
This file builds and runs a benchmark harness for the engine.

When you run:

make bench
this file is what executes.

It:

generates synthetic event streams
runs them through the matching engine
times each event
measures total runtime
calculates latency statistics
estimates peak memory usage
builds a latency histogram
writes results to benchmark_summary.json
Benchmark scenarios
The benchmark currently runs multiple scenarios, like:

Small correctness run
a small number of events
useful for quick sanity/performance check
Medium load
tens of thousands of events
Heavy load
hundreds of thousands of events
Why multiple scenarios?
Because performance can look different at different scales.

A tiny run might have:

more overhead noise
less representative latency
A large run shows:

sustained throughput
tail behavior
memory growth more clearly
What metrics it measures
1. Events processed
How many total events were handled.

Example:

500
50,000
500,000
This is just the workload size.

2. Total runtime
How long the whole scenario took.

Example:

93.8 ms
This tells you total wall-clock cost for the full scenario.

3. Throughput
How many events per second the engine processed.

Example:

5.3M events/s
This tells you how much work the engine can do over time.

Why it matters
It is the simplest speed number.

4. Average latency
Average time per event.

Example:

171 ns
This tells you typical per-event processing time.

Caution
Average is useful, but not enough by itself.

Because a system can have:

low average
but bad tail latency
5. Worst-case latency
Slowest event observed.

Example:

869542 ns
This shows the worst single spike in the run.

Why it matters
In low-latency systems, rare slow events can matter a lot.

6. p50 latency
Median latency.

This tells you:

what a typical event experiences
If p50 = 125 ns, then half the events were faster than that.

7. p99 latency
99th percentile latency.

This tells you:

how bad the slower tail is
Example:

p99 = 625 ns
Meaning:
99% of events were at or below 625 ns.

Why it matters
This is much more useful than just average when talking about latency-sensitive systems.

8. Peak memory
Estimated high-water memory usage during the run.

Example:

101.984 MB
This helps you understand:

memory footprint under load
whether the structure scales cleanly
9. Resting orders at end
How many orders remain in the book after the scenario finishes.

Why it matters
It gives context for the final state and the workload mix.

10. Latency histogram
The benchmark groups event latencies into buckets, such as:

<=100ns
101-250ns
251-500ns
501ns-1us
1-2us
2-5us
5-10us
>10us
Why this matters
This shows the full latency distribution, not just one number.

you can see where most events cluster
you can see whether the long tail is small or large
it makes the benchmark much more credible
What kind of events it benchmarks
The benchmark generates synthetic events such as:
limit adds
market orders
cancels
It uses random but deterministic generation.

Why deterministic matters
If the generator is seeded consistently, you can compare runs more fairly.
