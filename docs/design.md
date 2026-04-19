# Stage 0 Design

## Scope

- single instrument only
- central limit order book
- two sides: bid and ask
- price-time priority

## Supported messages

- limit order
- market order
- cancel order
- modify or replace order

## Core rules

- Limit orders may execute immediately against the opposite side.
- Any unfilled quantity from a limit order rests in the book.
- Market orders may execute immediately against the opposite side.
- Any unfilled quantity from a market order is discarded and never rests.
- Best price has priority over worse price.
- At the same price, earlier resting orders have priority over later orders.
- Partial fills are allowed.

## Data structures

- bid side stored as price levels ordered high to low
- ask side stored as price levels ordered low to high
- FIFO queue at each price level
- direct lookup index from `order_id` to the resting order location

## Worked scenarios

### Scenario 1

- Add `BUY 100 x 10`
- Add `SELL 101 x 5`
- No trade occurs.
- Book shows best bid `100 x 10` and best ask `101 x 5`.

### Scenario 2

- Resting book has `SELL 101 x 5`
- Incoming `BUY 102 x 8`
- Trade executes `5 @ 101`
- Remaining `BUY 102 x 3` rests in the bid book

### Scenario 3

- Resting book has:
  - `SELL 101 x 5` entered first
  - `SELL 101 x 7` entered second
- Incoming `BUY 101 x 6`
- First resting order fills completely for `5`
- Second resting order fills for `1`
- Remaining quantity on the second order is `6`
