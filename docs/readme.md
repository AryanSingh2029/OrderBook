docs/
Files:

design.md
matching_rules.md
benchmark_report.md
What this folder does:

defines the exchange rules before implementation
explains how orders should match
records performance results and optimization story
Why it matters:

this folder tells you what the engine is supposed to do
when you later read code, you can compare the implementation against these rules
this is the “specification + reasoning” layer of the project
How to think about each file:

design.md

this is the project blueprint
it defines the exchange assumptions:
central limit order book
price-time priority
order types
partial fills
resting behavior
if you want to understand the whole project at a high level, start here
matching_rules.md

this is more specific
it explains exactly how matching happens:
buy matches lowest ask
sell matches highest bid
better price first
FIFO within same price
this file is the behavior contract for the matching engine
benchmark_report.md

this is the performance story
it shows what was measured:
throughput
latency
memory
histogram
it also helps explain why Stage 9 optimization work mattered
What you should understand from docs/ before reading code:

what a limit order does
what a market order does
when an order rests in the book
how cancel works
why same-price orders use FIFO
what metrics/benchmarks the system tracks
So in one sentence:
docs/ is the theory/specification layer of the project; it tells you what the engine should do and how we evaluate it.


Limit order
A limit order says:

I want to buy or sell, but only at this price or better.

Examples:

BUY 100 @ 250
means buy 100 shares, but do not pay more than 250
SELL 50 @ 251
means sell 50 shares, but do not accept less than 251
What it does:

first tries to match against the opposite side
if it can trade at acceptable prices, it trades
if some quantity is left, the remaining part stays in the book
So a limit order can:

fully match
partially match and then rest
not match at all and rest fully
Market order
A market order says:

Execute immediately at the best available prices.

Examples:

BUY MARKET 100
SELL MARKET 50
What it does:

matches immediately against the opposite side
keeps consuming available liquidity until:
it is fully filled, or
the opposite book becomes empty
it never rests in the book
So market order = speed/guaranteed attempt to execute
limit order = price control

When an order rests in the book
An order “rests” when it is left waiting in the order book as visible liquidity.

This happens when:

it is a limit order
and it is not fully matched immediately
Examples:

No match:
book empty
BUY 100 @ 250
no seller available
order rests in bid side
Partial match:
ask side has SELL 30 @ 250
incoming BUY 100 @ 250
30 trades
remaining 70 rests in bid side
A market order does not rest.
Only leftover limit order quantity rests.

How cancel works
Cancel means:

Remove a live resting order from the book.

You give:

order_id
What happens:

if that order is still active in the book, it gets removed
if it was already filled, cancelled earlier, or never existed, cancel fails / not found
Example:

BUY order 101 @ 250 qty 100 is resting
CANCEL 101
order disappears from the book
Why this matters:

traders often pull liquidity before getting filled
HFT systems do this constantly
Why same-price orders use FIFO
FIFO = First In, First Out

At the same price, the earlier order gets priority.

Why?
Because your book uses price-time priority.

That means:

better price wins first
if price is equal, earlier time wins
Example:

order A: SELL 50 @ 251 entered first
order B: SELL 40 @ 251 entered second
incoming BUY 60 @ 251
What should happen:

first fill A for 50
then fill B for 10
Why this rule exists:

it is fair and deterministic
it rewards being first at that price
this is how many real markets work
This is also why queue position matters:
if you join a price level late, people ahead of you get filled first

What metrics the system tracks
Your project tracks several kinds of market/execution metrics.

Top-of-book metrics

best bid: highest current buy price
best ask: lowest current sell price
spread: best ask - best bid
mid price: average of best bid and best ask
These tell you the current market “headline.”

Depth metrics

quantity at top N bid levels
quantity at top N ask levels
cumulative depth
These tell you how much liquidity exists in the book.

Imbalance metrics

total bid volume vs ask volume
top-of-book imbalance
depth imbalance
These tell you whether the book is more buy-heavy or sell-heavy.

Trade metrics

traded volume
number of executions
VWAP
average trade size
These tell you what actually traded.

Execution metrics

fill ratio
slippage
queue depletion
These tell you execution quality.

Strategy metrics

inventory
realized PnL
unrealized PnL
adverse selection
These tell you how the toy strategy performed.

What benchmark/performance metrics the system tracks
Your benchmark system measures:

total events processed
total runtime
throughput in events/orders per second
average latency
worst-case latency
p50 latency
p99 latency
peak memory usage
latency histogram
Why this matters:

throughput tells how much work the engine can handle
latency tells how fast one event is processed
p99 tells tail behavior, which matters a lot in low-latency systems
memory matters because allocation/layout affect speed
histogram shows latency distribution, not just one average number
A short summary:

Limit order = trade only at a chosen price or better, may rest
Market order = trade immediately, never rests
Resting order = unmatched limit order sitting in the book
Cancel = remove an active resting order by ID
FIFO at same price = older orders get priority, because of price-time priority
Metrics = market state, liquidity, execution quality, strategy behavior, and performance measurements
