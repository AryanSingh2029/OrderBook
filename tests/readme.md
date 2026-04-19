tests/ is the correctness layer of the project.

This folder proves that the engine behaves the way we intended, not just that it “seems to work.”

Files in tests:

test_basic.cpp
test_matching.cpp
test_cancel.cpp
test_partial_fill.cpp
test_simulator.cpp
test_analytics.cpp
test_strategy.cpp
Think of it as:

tests/ = the proof that the system is correct

Why this folder matters
For an HFT-style project, correctness is extremely important.

A matching engine that is fast but wrong is useless.

These tests help prove:

order book state is correct
matching logic is correct
FIFO behavior is correct
cancel behavior is correct
replay behavior is deterministic
analytics are computed correctly
strategy layer behaves as expected

1. test_basic.cpp
This file checks the most fundamental book behavior.

What it tests
adding one buy order
adding one sell order
confirming book state after simple inserts
rejecting zero quantity
rejecting invalid price
rejecting duplicate order IDs
Why it matters
Before testing complex matching, we need to know the simplest cases are correct.

Mental model
This file answers:
Can the book accept valid orders and reject obviously bad ones?

2. test_matching.cpp
This tests the matching logic itself.

What it tests
full fill
partial fill
multiple matches across levels
market order behavior
FIFO behavior at same price
queue position behavior
Why it matters
This is one of the most important test files in the project.

It checks the core exchange behavior:

who matches with whom
in what order
how much quantity remains after trades
Mental model
This file answers:
Does the matching engine behave like a real price-time-priority order book?

3. test_cancel.cpp
This tests cancel behavior.

What it tests
cancel live order
cancel already filled order
cancel nonexistent order
Why it matters
Cancel is a core operation in real trading systems.

You need to know:

live resting orders can be removed
dead/nonexistent orders are handled safely
Mental model
This file answers:
Can the engine remove the right orders, and fail safely when it should?

4. test_partial_fill.cpp
This covers partial fill and replace/modify edge cases.

What it tests
partial fill correctness
replace order behavior
replace-to-zero behavior
invalid replace not destroying the live order
same-price replace resetting time priority
Why it matters
This file checks more subtle lifecycle behavior.

Especially important:
when an order is replaced, it often loses its old queue priority.
That is a realistic market behavior and a nice advanced touch.

Mental model
This file answers:
Do tricky partial-fill and replace cases behave correctly?

5. test_simulator.cpp
This tests the replay/event-driven layer.

What it tests
loading and replaying events
simulator correctness
deterministic replay
multi-instrument separation
Why it matters
Even if the matching engine is correct, the simulator must also replay events correctly.

This is important because the project is not just a book, it is an event-driven system.

Mental model
This file answers:
Does feeding the same event stream always produce the same correct result, and do symbols stay separate?

6. test_analytics.cpp
This tests the analytics layer.

What it tests
top-of-book metrics
depth calculations
imbalance calculations
VWAP
average trade size
fill ratio
slippage
queue depletion
Why it matters
Analytics are easy to get subtly wrong.

This file makes sure the microstructure/statistics layer is trustworthy.

Mental model
This file answers:
Are the numbers we compute from the book and trades actually correct?

7. test_strategy.cpp
This tests the toy strategy layer.

What it tests
strategy execution path
signal triggering
fills
stats tracking
Why it matters
The strategy is not the star of the project, but once it exists, it should still behave consistently.

Mental model
This file answers:
Does the strategy layer run sensibly on top of the engine and report correct summary stats?

How to think about the test structure overall
The tests are layered in a good order:

test_basic.cpp
basic validity

test_matching.cpp
core matching logic

test_cancel.cpp
order removal

test_partial_fill.cpp
more subtle lifecycle/replace cases

test_simulator.cpp
event-driven replay correctness

test_analytics.cpp
market/execution metric correctness

test_strategy.cpp
top-level strategy behavior

That is a strong structure because it tests from the bottom up.

What make test does
When you run:

make test
it builds and runs all these test executables.

If everything is good, you get outputs like:

test_basic passed
test_matching passed
etc.
So make test is the main correctness checkpoint for the whole project.


Simple summary
test_basic.cpp
validates simple inserts and invalid inputs

test_matching.cpp
proves the core matching engine works

test_cancel.cpp
proves cancel behavior works

test_partial_fill.cpp
proves subtle fill/replace behavior works

test_simulator.cpp
proves replay and multi-symbol logic work

test_analytics.cpp
proves metrics are correct

test_strategy.cpp
proves the toy strategy layer behaves properly

So in one line:

tests/ is where the project proves that the engine, replay system, analytics, and strategy all behave correctly.
