src/ is where the real behavior lives.

If include/ tells you what exists, src/ tells you:

how it works
how data moves
how orders get matched
how replay, logging, analytics, and strategy actually run
Files in src:

order.cpp
order_book.cpp
matching_engine.cpp
exchange.cpp
simulator.cpp
analytics.cpp
reporting.cpp
strategy.cpp
utils.cpp
main.cpp
strategy_main.cpp
1. order.cpp
This is usually the lightest file.

What it does
It implements small helpers tied to the basic types in order.h, such as:

string conversion helpers
tiny utility behavior around enums or simple structs
Why it exists
To keep basic type-related logic out of larger files.

How to think about it
This file is not the engine.
It just supports the rest of the system.

2. order_book.cpp
This is the heart of the project.

If you want to understand the actual order book, this is the most important file.

What it does
It implements the OrderBook:

adding limit orders
adding market orders
matching against opposite side
resting unmatched quantity
canceling orders
modifying orders
maintaining bid/ask books
maintaining order lookup
queue position tracking
generating trades
Core responsibilities
This file manages:

bid side price levels
ask side price levels
slot-pool order storage
order_id -> order location
FIFO behavior within a price level
Most important ideas inside
A. Matching logic
When a new order comes in:

if it can cross the opposite side, it trades
it keeps trading until:
it is fully filled, or
no more valid opposite-side liquidity exists
B. Resting logic
If a limit order still has quantity left:

it gets inserted into the correct price level
it becomes part of the book
C. Market order logic
If a market order has quantity left:

it does not rest
leftover quantity is discarded
D. Price levels
Each price level behaves like a FIFO queue:

older orders first
newer orders behind them
E. Slot pool
Instead of heavy per-order linked-list allocation, the file uses a reusable slot-based structure.
That helps reduce allocation overhead and improves latency.

Why this file matters
This is where the actual “exchange book mechanics” happen.


3. matching_engine.cpp
This file wraps the raw book with higher-level order lifecycle behavior.

What it does
It processes requests like:

submit limit order
submit market order
cancel
replace
And tracks order state transitions such as:

active
filled
cancelled
replaced
rejected
Why it exists separately from order_book.cpp
Because the order book’s job is:

maintain market state
perform matching/storage
The matching engine’s job is:

produce higher-level engine results
track order lifecycle
create state updates for debugging/logging
Key thing to notice
This file gives you a cleaner external interface for “place an order” than directly calling into the book.

Mental model
order_book.cpp = raw mechanism
matching_engine.cpp = engine behavior + status tracking

4. exchange.cpp
This file makes the project multi-instrument.

What it does
It holds one MatchingEngine per symbol and routes events:

AAPL orders go to one engine
MSFT orders go to another
cancel searches by global order ID ownership
Why it matters
Without this, the system is one book.
With this, it starts feeling like an exchange router.

Important idea
Each symbol has:

separate state
separate top of book
separate trades
separate depth
So this file ensures books do not interfere with each other.

5. simulator.cpp
This file turns input data into replayed market behavior.

What it does
It:

parses CSV event streams
parses manual command-style inputs
replays events in timestamp order
calls the Exchange
collects replay results
computes per-event snapshots and metrics
Why it matters
This is what makes your project event-driven rather than just “manually calling functions.”

Important idea
For each event, it records:

the event itself
what happened in the engine
book snapshots
top of book before/after
analytics after processing
So this file is the replay orchestrator.

6. analytics.cpp
This file computes the market and execution metrics.

What it does
It calculates:

best bid / best ask
spread
mid
top-N depth
depth imbalance
total bid/ask volume
VWAP
average trade size
fill ratio
slippage
queue depletion
Why it matters
This turns the project from “just data structures” into “microstructure-aware system.”

Important design point
The analytics are computed from book state and trade output rather than being mixed into core matching code.
That separation is a good design decision.

7. reporting.cpp
This file turns replay results into logs and summaries.

What it does
It writes:

trades.csv
rejections.csv
snapshots.csv
states.csv
metrics.csv
It also prints:

end-of-run summary
final top levels
Why it matters
This is your observability layer.

It helps you:

debug behavior
inspect outputs
feed the UI
create structured artifacts for analysis
One especially important point
The UI depends on this file.
So reporting.cpp is the bridge between backend replay and visual understanding.

8. strategy.cpp
This file implements the toy strategy sandbox.

What it does
It runs a simple imbalance-based taker strategy:

reads market state after events
decides whether to buy or sell
sends small market orders
tracks inventory and PnL
Why it matters
It demonstrates how trading logic sits on top of the order book.

Important note
This file is intentionally not the main focus.
It’s there to show extension capability, not to overshadow the engine.

9. utils.cpp
This file contains helper functions.

What it does
Things like:

parsing CSV pieces
string splitting
enum parsing
utility conversion helpers
Why it exists
To keep the main logic files clean and focused.

10. main.cpp
This is the main replay executable.

What it does
When you run:

make run
this is the file that runs.

It:

loads the sample event file
runs the simulator
prints per-event debug output
writes logs
prints end-of-run summary
Why it matters
This is the easiest non-UI entry point into the project.

Mental model
This file is the “demo runner” for the engine.

11. strategy_main.cpp
This is the standalone runner for the strategy sandbox.

What it does
When you run:

./build/strategy_demo
this file:

loads replay events
runs the strategy
prints strategy summary
writes strategy_summary.json
Why it exists
To keep strategy execution separate from the main engine replay.

That separation is good because:

engine remains the main star
strategy stays optional and modular
How all src/ files connect
The whole runtime flow is basically:

main.cpp
starts the replay

simulator.cpp
loads and replays events

exchange.cpp
routes each event by symbol

matching_engine.cpp
processes order lifecycle requests

order_book.cpp
does the real matching and state mutation

analytics.cpp
computes market/execution metrics

reporting.cpp
writes logs and summaries

ui/
reads those logs and visualizes them

And separately:

strategy_main.cpp + strategy.cpp
run the toy trading layer
benchmark.cpp
measures performance
If you only focus on 4 files first
For understanding the engine deeply, prioritize:

main.cpp
simulator.cpp
exchange.cpp
order_book.cpp
That gives you the actual path from input event to market state change.
