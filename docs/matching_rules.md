# Matching Rules

## Priority model

- Buy orders match the lowest available ask first.
- Sell orders match the highest available bid first.
- Better price always wins over worse price.
- At the same price, older resting orders match before newer resting orders.

## Order behavior

- A limit buy crosses when `buy_price >= best_ask`.
- A limit sell crosses when `sell_price <= best_bid`.
- A market order crosses any available opposite-side liquidity.
- Market orders do not rest in the book.
- Limit orders rest only if unfilled quantity remains after matching.

## Cancel and replace

- Cancel removes a resting order by `order_id`.
- Replace is modeled as cancel plus new add with a fresh priority timestamp.
- Replace with quantity `0` behaves like cancel.

## Current assumptions

- single symbol only
- no hidden orders
- no iceberg orders
- no self-trade prevention
- no auction phases
- no fees or rebates
