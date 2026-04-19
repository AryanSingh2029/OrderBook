include/ is the interface layer of the project.

This folder tells you:

what objects exist in the system
what data each object stores
what operations are allowed
how the main modules talk to each other
A good way to think about it is:

include/ = the project’s vocabulary and contracts

Files in include:

order.h
order_book.h
matching_engine.h
analytics.h
exchange.h
simulator.h
reporting.h
strategy.h
utils.h
1. order.h
This is the most basic data-model file.

It defines the fundamental types used everywhere:

OrderId
Price
Quantity
Timestamp
It also defines enums like:

Side -> Buy or Sell
OrderType -> Limit or Market
And the main structs:

Order
Trade
AddResult
Why it exists
Every other part of the system needs a shared definition of:

what an order is
what a trade is
what types prices/quantities use
Mental model
This file is the project’s “primitive building blocks.”

2. order_book.h
This is one of the most important files in the whole project.

It defines:

LevelSnapshot
PriceLevel
TopOfBook
OrderMetadata
QueuePosition
OrderBook class
What OrderBook is responsible for
OrderBook stores market state.

That means:

bid levels
ask levels
resting orders
order lookup map
queue position info
top-of-book info
Main methods
add_limit_order(...)
add_market_order(...)
cancel_order(...)
modify_order(...)
top_of_book()
levels(...)
order_metadata(...)
queue_position(...)
total_volume(...)
Why this file matters
If you want to understand “where the market lives,” it’s here.

Very important design idea
OrderBook is about state, not orchestration.

It knows:

what orders are resting
what price levels exist
how to snapshot them
It does not own the higher-level replay flow.

3. matching_engine.h
This sits one level above the order book.

It defines:

OrderStatus
OrderState
EngineResult
MatchingEngine class
What MatchingEngine does
This is the logic layer that processes requests like:

submit limit order
submit market order
replace order
cancel order
What it adds on top of OrderBook
The OrderBook stores the raw state, but the matching engine also tracks:

order lifecycle status
Active
Filled
Cancelled
Replaced
Rejected
per-event outputs
state updates for logging/debugging
Main methods
submit_limit_order(...)
submit_market_order(...)
replace_order(...)
cancel_order(...)
order_state(...)
order_book()
Mental model
OrderBook = market state
MatchingEngine = order processing logic around that state

4. analytics.h
This file defines the analytics layer.

It contains metric structs for:

top-of-book metrics
depth metrics
imbalance metrics
trade metrics
execution metrics
market metrics
And an Analytics class with methods that compute them.

What it does
It takes book state or trade results and computes useful quantities like:

spread
mid
total bid/ask volume
imbalance
VWAP
fill ratio
slippage
queue depletion
Why it exists
This keeps analytics separate from the matching code.

That is good design because:

engine code stays focused on correctness
analytics code stays focused on interpretation
5. exchange.h
This is the multi-instrument routing layer.

It defines:

RoutedResult
Exchange class
What Exchange does
It manages multiple books, one per symbol.

For example:

AAPL -> one MatchingEngine
MSFT -> another MatchingEngine
Main methods
submit_limit_order(symbol, ...)
submit_market_order(symbol, ...)
cancel_order(order_id)
engine_for(symbol)
symbols()
total_resting_orders()
Why it matters
Before this, the project was one book.
Now it behaves more like an exchange system.

Mental model
Exchange is the router:
it sends each symbol’s events to the correct matching engine.

6. simulator.h
This is the event-replay interface.

It defines:

EventType
Event
ReplayResult
Simulator class
What Event represents
One line of market input, such as:

add
market
cancel
with fields like:

timestamp
symbol
order id
side
price
quantity
What ReplayResult represents
The full result of processing one event:

original event
routed engine result
pre-event top of book
post-event top of book
level snapshots
market metrics
trade metrics
execution metrics
Main methods
load_events(...)
load_commands(...)
replay(...)
Mental model
This file defines the bridge between input data and engine behavior.

7. reporting.h
This is the output/logging layer.

It defines:

RunSummary
ReplayReporter
What it does
It turns replay results into:

human-readable summaries
CSV logs
top-level final state output
Main methods
summarize(...)
write_trade_log(...)
write_rejection_log(...)
write_snapshot_log(...)
write_state_log(...)
write_metrics_log(...)
print_top_levels(...)
print_summary(...)
Why it matters
Without this file, the engine would run, but it would be harder to inspect.

This is what makes the project feel engineered.

8. strategy.h
This is the strategy sandbox layer.

It defines:

strategy result/stat structs
the toy strategy class
What it does
Runs a simple trading logic on top of replayed market state.

It tracks:

inventory
fill ratio
realized/unrealized PnL
adverse selection
Why it exists
To show how a trading idea can sit on top of the order book, without making strategy the main project.

9. utils.h
This is helper functionality.

It usually contains
parsers
string helpers
CSV splitting
enum conversion helpers
Why it exists
To keep the main engine files cleaner and avoid repeating helper logic everywhere.