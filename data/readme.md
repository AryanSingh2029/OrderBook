data/ is the input side of the project.

This folder contains the event streams and sample inputs that feed the engine.

Files in data:

sample_events.csv
benchmark_events.csv
sample_commands.txt
Think of this folder as:

data/ = what we feed into the system

1. sample_events.csv
This is the most important input file for understanding the project.

When you run:

make run
the main replay program reads this file.

Format
The CSV format is:

timestamp,symbol,type,side,order_id,price,quantity
Example rows:

1,AAPL,ADD,BUY,101,100,250
2,AAPL,ADD,SELL,102,101,50
3,AAPL,ADD,SELL,103,100,70
4,AAPL,CANCEL,,101,,
5,MSFT,ADD,BUY,201,200,40
What each field means
timestamp
when the event happens in replay order
symbol
which instrument/book it belongs to
type
what kind of event it is:
ADD
MARKET
CANCEL
side
BUY or SELL
order_id
unique identifier for that order
price
relevant for limit orders
quantity
order size
Why it matters
This file is the easiest way to understand the engine because it gives a concrete sequence of market actions.

How to think about it
Each row is one message entering the exchange.

The engine processes them one by one and updates the book.

2. benchmark_events.csv
This is intended as input for performance-style runs or larger synthetic scenarios.

What it represents
It gives a larger event stream than the tiny demo sample.

The goal is not mainly visual understanding, but more:

throughput testing
heavier replay
performance experimentation
Why it exists
You do not want to benchmark only a tiny 5-6 event file.
A somewhat larger event file gives more realistic stress.

Mental model
If sample_events.csv is for learning,
benchmark_events.csv is for load/performance-style experimentation.

3. sample_commands.txt
This is the manual command-style input format.

It is different from the CSV event format.

Instead of structured CSV rows, it uses command-like lines such as:

BUY LIMIT 101 100 250
SELL LIMIT 102 101 50
CANCEL 101
Why this exists
This file is useful for:

quick demos
human-readable testing
understanding logic without CSV syntax
How to think about it
This is more like a “toy shell input” for the matching engine.

It is easier for humans to read, but less realistic than timestamped event replay.

Why the data/ folder matters
This folder is important because it separates:

the engine logic
from
the input scenarios
That is good design.

Instead of hardcoding events in C++ code, we can:

swap input files
replay different scenarios
test edge cases
benchmark larger streams
feed the UI with reproducible sequences
So data/ makes the project:

deterministic
inspectable
easy to experiment with
Best way to understand data/
If you want to really connect input to behavior:

open sample_events.csv
read one row
predict what should happen
run make run
compare your prediction with the output
then check the UI
That is one of the best learning loops for this project.

Simple mental summary
sample_events.csv
main event-driven replay input

benchmark_events.csv
heavier performance/load-style input

sample_commands.txt
human-friendly command examples
So in one line:
data/ contains the market events and commands that drive the whole system.
