logs/ is the output side of the project.

If data/ is what goes into the engine, logs/ is what comes out after replay, strategy, and benchmark runs.

Folder: logs

Current files:

trades.csv
snapshots.csv
rejections.csv
states.csv
metrics.csv
strategy_summary.json
benchmark_summary.json
Think of it as:

logs/ = everything the system records so you can inspect, debug, analyze, and visualize it

1. trades.csv
This file records every trade that actually happened.

What it contains
Each row is one execution.

Typical fields:

event_timestamp
symbol
buy_order_id
sell_order_id
trade_price
trade_quantity
trade_timestamp
Why it matters
This tells you:

which orders matched
at what price
for how much quantity
during which replay event
How to think about it
If you want to know:
Did a trade happen? Who traded with whom?
this is the file to read.

2. snapshots.csv
This file records the visible order book after each replayed event.

What it contains
Each row is one price level snapshot.

Fields:

event_timestamp
symbol
side
level_index
price
quantity
order_count
Why it matters
This tells you what the book looked like after an event.

For example:

best bid level
second ask level
quantity at each visible level
how many orders are sitting at that price
How to think about it
This file is the book’s “frame-by-frame state.”

The UI uses this heavily.

3. rejections.csv
This file records invalid or rejected events.

What it contains
Fields:

event_timestamp
symbol
event_type
order_id
reason
Why it matters
Not every input should be accepted.

Examples of rejected things:

duplicate order ID
zero quantity
invalid price
How to think about it
This is your error/debugging log for bad inputs.

If something does not enter the engine, this file tells you why.

4. states.csv
This file records order state updates and queue position information.

What it contains
Fields:

event_timestamp
symbol
order_id
status
remaining_quantity
orders_ahead
quantity_ahead
level_order_count
level_total_quantity
Why it matters
This is one of the most HFT-relevant logs.

It tells you:

whether an order became ACTIVE, FILLED, etc.
how much quantity is left
where that order sits in the queue
how much size/orders are ahead of it
How to think about it
If trades.csv tells you what executed,
states.csv tells you what happened to orders themselves.

This is especially useful for:

queue position understanding
debugging order lifecycle
explaining price-time priority
5. metrics.csv
This file records market and execution analytics after each event.

What it contains
Fields like:

best_bid
best_ask
spread
mid
bid_top_depth
ask_top_depth
total_bid
total_ask
top_imbalance
depth_imbalance
traded_volume
execution_count
fill_ratio
slippage
queue_depletion
Why it matters
This file turns raw book state into interpretable market metrics.

How to think about it
If you want to analyze:

how spread changes over time
whether bids dominate asks
whether execution quality was good or bad
this is the file to use.

The UI’s microstructure panels and timeline come from here.

6. strategy_summary.json
This file records the toy strategy’s final summary.

It is produced when you run:

./build/strategy_demo
What it contains
Things like:

external events processed
signals triggered
orders sent
filled orders
requested qty
filled qty
fill ratio
inventory
average cost
realized PnL
unrealized PnL
total PnL
adverse selection stats
Why it matters
This tells you how the strategy performed on top of the engine.

How to think about it
This is the strategy’s scoreboard.

7. benchmark_summary.json
This file records benchmark results.

It is produced when you run:

make bench
What it contains
For each scenario:

scenario name
events processed
total runtime
throughput
average latency
worst-case latency
p50
p99
peak memory
resting orders at end
latency histogram buckets
Why it matters
This is your performance report in machine-readable form.

How to think about it
If metrics.csv is market analytics,
benchmark_summary.json is system-performance analytics.

The benchmark UI panel reads from here.

Why logs/ is important
This folder makes the project feel professional because it gives you:

observability
reproducibility
structured outputs
debugging support
analysis-ready data
UI-ready data
Without logs/, the engine would still work, but it would be much harder to:

inspect behavior
debug strange events
visualize outcomes
explain the project clearly
Simple mental mapping
trades.csv
what got executed

snapshots.csv
what the book looked like

rejections.csv
what got refused

states.csv
what happened to individual orders

metrics.csv
what the market/execution metrics were

strategy_summary.json
how the toy strategy performed

benchmark_summary.json
how fast the system ran

Best way to use logs/ to understand the system
For one replay event, look in this order:

sample_events.csv
what came in

trades.csv
did it trade?

states.csv
what happened to the orders?

snapshots.csv
what does the book look like now?

metrics.csv
what does that state mean in market terms?

That sequence gives you a complete picture.
