const DATA_FILES = {
  events: "../data/sample_events.csv",
  trades: "../logs/trades.csv",
  snapshots: "../logs/snapshots.csv",
  rejections: "../logs/rejections.csv",
  states: "../logs/states.csv",
  metrics: "../logs/metrics.csv",
  benchmark: "../logs/benchmark_summary.json",
  strategy: "../logs/strategy_summary.json",
};

const state = {
  events: [],
  trades: [],
  snapshots: [],
  rejections: [],
  states: [],
  metrics: [],
  benchmark: null,
  strategy: null,
  symbols: [],
  selectedSymbol: "",
  selectedIndex: 0,
  playing: false,
  timer: null,
};

const els = {
  symbolSelect: document.querySelector("#symbol-select"),
  eventSlider: document.querySelector("#event-slider"),
  eventCounter: document.querySelector("#event-counter"),
  eventDetails: document.querySelector("#event-details"),
  topMetrics: document.querySelector("#top-metrics"),
  depthMetrics: document.querySelector("#depth-metrics"),
  askBody: document.querySelector("#ask-body"),
  bidBody: document.querySelector("#bid-body"),
  tradeBody: document.querySelector("#trade-body"),
  recentTradesBody: document.querySelector("#recent-trades-body"),
  diffPanel: document.querySelector("#event-diff"),
  queuePanel: document.querySelector("#queue-panel"),
  rejectionPanel: document.querySelector("#rejection-panel"),
  timelineChart: document.querySelector("#timeline-chart"),
  heatmap: document.querySelector("#heatmap"),
  strategyPanel: document.querySelector("#strategy-panel"),
  benchmarkPanel: document.querySelector("#benchmark-panel"),
  prevBtn: document.querySelector("#prev-btn"),
  nextBtn: document.querySelector("#next-btn"),
  playBtn: document.querySelector("#play-btn"),
};

boot().catch((error) => {
  console.error(error);
  document.body.innerHTML = `<pre style="padding:24px">Failed to load replay UI.\n${error.message}</pre>`;
});

async function boot() {
  const [eventsCsv, tradesCsv, snapshotsCsv, rejectionsCsv, statesCsv, metricsCsv, benchmarkJson, strategyJson] =
    await Promise.all([
      fetchText(DATA_FILES.events),
      fetchText(DATA_FILES.trades),
      fetchText(DATA_FILES.snapshots),
      fetchText(DATA_FILES.rejections),
      fetchText(DATA_FILES.states),
      fetchText(DATA_FILES.metrics),
      fetchOptionalJson(DATA_FILES.benchmark),
      fetchOptionalJson(DATA_FILES.strategy),
    ]);

  state.events = parseCsv(eventsCsv).map((row) => ({
    timestamp: Number(row.timestamp),
    symbol: row.symbol,
    type: row.type,
    side: row.side || "",
    orderId: row.order_id,
    price: row.price || "",
    quantity: row.quantity || "",
  }));

  state.trades = parseCsv(tradesCsv).map((row) => ({
    eventTimestamp: Number(row.event_timestamp),
    symbol: row.symbol,
    buyOrderId: row.buy_order_id,
    sellOrderId: row.sell_order_id,
    price: Number(row.trade_price),
    quantity: Number(row.trade_quantity),
  }));

  state.snapshots = parseCsv(snapshotsCsv).map((row) => ({
    eventTimestamp: Number(row.event_timestamp),
    symbol: row.symbol,
    side: row.side,
    levelIndex: Number(row.level_index),
    price: Number(row.price),
    quantity: Number(row.quantity),
    orderCount: Number(row.order_count),
  }));

  state.rejections = parseCsv(rejectionsCsv).map((row) => ({
    eventTimestamp: Number(row.event_timestamp),
    symbol: row.symbol,
    type: row.event_type,
    orderId: row.order_id,
    reason: row.reason,
  }));

  state.states = parseCsv(statesCsv).map((row) => ({
    eventTimestamp: Number(row.event_timestamp),
    symbol: row.symbol,
    orderId: row.order_id,
    status: row.status,
    remainingQuantity: row.remaining_quantity ? Number(row.remaining_quantity) : null,
    ordersAhead: row.orders_ahead ? Number(row.orders_ahead) : null,
    quantityAhead: row.quantity_ahead ? Number(row.quantity_ahead) : null,
    levelOrderCount: row.level_order_count ? Number(row.level_order_count) : null,
    levelTotalQuantity: row.level_total_quantity ? Number(row.level_total_quantity) : null,
  }));

  state.metrics = parseCsv(metricsCsv).map((row) => ({
    eventTimestamp: Number(row.event_timestamp),
    symbol: row.symbol,
    bestBid: toNumberOrNull(row.best_bid),
    bestAsk: toNumberOrNull(row.best_ask),
    spread: toNumberOrNull(row.spread),
    mid: toNumberOrNull(row.mid),
    bidDepth: Number(row.bid_top_depth || 0),
    askDepth: Number(row.ask_top_depth || 0),
    totalBid: Number(row.total_bid || 0),
    totalAsk: Number(row.total_ask || 0),
    topImbalance: toNumberOrNull(row.top_imbalance),
    depthImbalance: toNumberOrNull(row.depth_imbalance),
    tradedVolume: Number(row.traded_volume || 0),
    executionCount: Number(row.execution_count || 0),
    fillRatio: Number(row.fill_ratio || 0),
    slippage: toNumberOrNull(row.slippage),
    queueDepletion: Number(row.queue_depletion || 0),
  }));

  state.benchmark = benchmarkJson;
  state.strategy = strategyJson;
  state.symbols = [...new Set(state.events.map((event) => event.symbol))];
  state.selectedSymbol = state.symbols[0] || "";

  bindEvents();
  populateSymbols();
  refreshEventBounds();
  render();
}

function bindEvents() {
  els.symbolSelect.addEventListener("change", () => {
    state.selectedSymbol = els.symbolSelect.value;
    state.selectedIndex = 0;
    refreshEventBounds();
    render();
  });

  els.eventSlider.addEventListener("input", () => {
    state.selectedIndex = Number(els.eventSlider.value);
    render();
  });

  els.prevBtn.addEventListener("click", () => {
    pause();
    state.selectedIndex = Math.max(0, state.selectedIndex - 1);
    render();
  });

  els.nextBtn.addEventListener("click", () => {
    pause();
    state.selectedIndex = Math.min(filteredEvents().length - 1, state.selectedIndex + 1);
    render();
  });

  els.playBtn.addEventListener("click", () => {
    state.playing ? pause() : play();
  });
}

function play() {
  pause();
  state.playing = true;
  els.playBtn.textContent = "Pause";
  state.timer = window.setInterval(() => {
    const lastIndex = filteredEvents().length - 1;
    if (state.selectedIndex >= lastIndex) {
      pause();
      return;
    }
    state.selectedIndex += 1;
    render();
  }, 900);
}

function pause() {
  state.playing = false;
  els.playBtn.textContent = "Play";
  if (state.timer !== null) {
    window.clearInterval(state.timer);
    state.timer = null;
  }
}

function populateSymbols() {
  els.symbolSelect.innerHTML = state.symbols
    .map((symbol) => `<option value="${symbol}">${symbol}</option>`)
    .join("");
  els.symbolSelect.value = state.selectedSymbol;
}

function refreshEventBounds() {
  const count = filteredEvents().length;
  els.eventSlider.max = Math.max(0, count - 1);
  els.eventSlider.value = Math.min(state.selectedIndex, Math.max(0, count - 1));
  state.selectedIndex = Number(els.eventSlider.value);
}

function render() {
  const events = filteredEvents();
  if (events.length === 0) {
    return;
  }

  const currentEvent = events[state.selectedIndex];
  const prevEvent = events[Math.max(0, state.selectedIndex - 1)];
  const currentTimestamp = currentEvent.timestamp;
  const prevTimestamp = state.selectedIndex > 0 ? prevEvent.timestamp : null;

  els.eventSlider.value = String(state.selectedIndex);
  els.eventCounter.textContent = `Event ${state.selectedIndex + 1} / ${events.length}`;

  const book = snapshotFor(state.selectedSymbol, currentTimestamp);
  const previousBook = prevTimestamp === null
    ? { bids: [], asks: [] }
    : snapshotFor(state.selectedSymbol, prevTimestamp);
  const eventTrades = state.trades.filter(
    (trade) => trade.symbol === state.selectedSymbol && trade.eventTimestamp === currentTimestamp,
  );
  const recentTrades = state.trades
    .filter((trade) => trade.symbol === state.selectedSymbol && trade.eventTimestamp <= currentTimestamp)
    .slice(-6)
    .reverse();
  const rejection = state.rejections.find(
    (item) => item.symbol === state.selectedSymbol && item.eventTimestamp === currentTimestamp,
  );
  const statesAtEvent = state.states.filter(
    (item) => item.symbol === state.selectedSymbol && item.eventTimestamp === currentTimestamp,
  );
  const metricsTimeline = state.metrics.filter((metric) => metric.symbol === state.selectedSymbol);
  const metricAtEvent = metricsTimeline.find((metric) => metric.eventTimestamp === currentTimestamp) || null;

  const changedBidKeys = changedLevelKeys(previousBook.bids, book.bids, "BID");
  const changedAskKeys = changedLevelKeys(previousBook.asks, book.asks, "ASK");

  renderEventDetails(currentEvent, rejection);
  renderTopMetrics(book);
  renderDepthMetrics(book, metricAtEvent);
  renderEventDiff(previousBook, book);
  renderQueuePanel(statesAtEvent);
  renderRejections(currentTimestamp);
  renderBookRows(els.bidBody, book.bids, "bid-row", changedBidKeys, "BID");
  renderBookRows(els.askBody, book.asks, "ask-row", changedAskKeys, "ASK");
  renderTrades(els.tradeBody, eventTrades, "No trades at this event.");
  renderTrades(els.recentTradesBody, recentTrades, "No trades yet.", true);
  renderTimeline(metricsTimeline, currentTimestamp);
  renderHeatmap(book);
  renderStrategyPanel();
  renderBenchmarkPanel();
}

function renderEventDetails(event, rejection) {
  const details = [
    ["Timestamp", event.timestamp],
    ["Symbol", event.symbol],
    ["Type", event.type],
    ["Side", event.side || "NA"],
    ["Order ID", event.orderId],
    ["Price", event.price || "NA"],
    ["Quantity", event.quantity || "NA"],
    ["Status", rejection ? `Rejected: ${rejection.reason}` : "Accepted / processed"],
  ];

  els.eventDetails.innerHTML = details
    .map(([label, value]) => `<div class="detail-item"><span>${label}</span><strong>${value}</strong></div>`)
    .join("");
}

function renderTopMetrics(book) {
  const bestBid = book.bids[0];
  const bestAsk = book.asks[0];
  const spread = bestBid && bestAsk ? bestAsk.price - bestBid.price : "NA";
  const mid = bestBid && bestAsk ? ((bestBid.price + bestAsk.price) / 2).toFixed(2) : "NA";

  const items = [
    ["Best Bid", bestBid ? `${bestBid.price} x ${bestBid.quantity}` : "NA"],
    ["Best Ask", bestAsk ? `${bestAsk.price} x ${bestAsk.quantity}` : "NA"],
    ["Spread", spread],
    ["Mid", mid],
  ];

  els.topMetrics.innerHTML = items
    .map(([label, value]) => `<div class="metric-item"><span>${label}</span><strong>${value}</strong></div>`)
    .join("");
}

function renderDepthMetrics(book, metricAtEvent) {
  const bidDepth = book.bids.reduce((sum, level) => sum + level.quantity, 0);
  const askDepth = book.asks.reduce((sum, level) => sum + level.quantity, 0);
  const total = bidDepth + askDepth;
  const imbalance = metricAtEvent?.depthImbalance ?? (total > 0 ? (bidDepth - askDepth) / total : null);

  const items = [
    ["Bid Top Levels Qty", bidDepth],
    ["Ask Top Levels Qty", askDepth],
    ["Depth Imbalance", imbalance == null ? "NA" : imbalance.toFixed(4)],
    ["Visible Levels", `${book.bids.length} bid / ${book.asks.length} ask`],
  ];

  els.depthMetrics.innerHTML = items
    .map(([label, value]) => `<div class="metric-item"><span>${label}</span><strong>${value}</strong></div>`)
    .join("");
}

function renderEventDiff(previousBook, currentBook) {
  const diffItems = [];

  const sides = [
    ["BID", previousBook.bids, currentBook.bids],
    ["ASK", previousBook.asks, currentBook.asks],
  ];

  for (const [side, previousLevels, currentLevels] of sides) {
    const previousMap = new Map(previousLevels.map((level) => [level.levelIndex, level]));
    const currentMap = new Map(currentLevels.map((level) => [level.levelIndex, level]));
    const keys = [...new Set([...previousMap.keys(), ...currentMap.keys()])].sort((a, b) => a - b);

    for (const key of keys) {
      const prev = previousMap.get(key);
      const curr = currentMap.get(key);
      if (!prev && curr) {
        diffItems.push({ kind: "positive", text: `${side} L${key} added at ${curr.price} for ${curr.quantity}` });
      } else if (prev && !curr) {
        diffItems.push({ kind: "negative", text: `${side} L${key} removed from ${prev.price}` });
      } else if (prev && curr && (prev.price !== curr.price || prev.quantity !== curr.quantity || prev.orderCount !== curr.orderCount)) {
        diffItems.push({
          kind: "neutral",
          text: `${side} L${key} changed ${prev.price}/${prev.quantity} -> ${curr.price}/${curr.quantity}`,
        });
      }
    }
  }

  if (diffItems.length === 0) {
    els.diffPanel.innerHTML = `<div class="stack-item diff-neutral"><span>No visible book change</span><strong>The event did not change visible depth.</strong></div>`;
    return;
  }

  els.diffPanel.innerHTML = diffItems
    .map((item) => `<div class="stack-item diff-${item.kind}"><span>Book diff</span><strong>${item.text}</strong></div>`)
    .join("");
}

function renderQueuePanel(statesAtEvent) {
  const activeStates = statesAtEvent.filter((item) => item.status === "ACTIVE");
  if (activeStates.length === 0) {
    els.queuePanel.innerHTML = `<div class="stack-item"><span>No active queue view</span><strong>No active orders were updated at this event.</strong></div>`;
    return;
  }

  els.queuePanel.innerHTML = activeStates
    .map((item) => `
      <div class="stack-item">
        <span>Order ${item.orderId}</span>
        <strong>ahead_orders=${item.ordersAhead ?? "NA"} ahead_qty=${item.quantityAhead ?? "NA"}</strong>
        <span>level_orders=${item.levelOrderCount ?? "NA"} level_qty=${item.levelTotalQuantity ?? "NA"} remaining=${item.remainingQuantity ?? "NA"}</span>
      </div>
    `)
    .join("");
}

function renderRejections(currentTimestamp) {
  const symbolRejections = state.rejections
    .filter((item) => item.symbol === state.selectedSymbol && item.eventTimestamp <= currentTimestamp)
    .slice(-6)
    .reverse();

  if (symbolRejections.length === 0) {
    els.rejectionPanel.innerHTML = `<div class="stack-item"><span>No rejections</span><strong>This symbol has no rejected events.</strong></div>`;
    return;
  }

  els.rejectionPanel.innerHTML = symbolRejections
    .map((item) => `
      <div class="stack-item rejection">
        <span>Event ${item.eventTimestamp}${item.eventTimestamp === currentTimestamp ? " (current)" : ""}</span>
        <strong>${item.type} order ${item.orderId}</strong>
        <span>${item.reason}</span>
      </div>
    `)
    .join("");
}

function renderBookRows(container, levels, className, changedKeys, side) {
  if (levels.length === 0) {
    container.innerHTML = `<tr><td class="empty" colspan="4">No levels</td></tr>`;
    return;
  }

  container.innerHTML = levels
    .map((level, index) => {
      const changed = changedKeys.has(`${side}:${level.levelIndex}`) ? "changed-row" : "";
      return `
        <tr class="${className} ${changed}">
          <td>${index}</td>
          <td>${level.price}</td>
          <td>${level.quantity}</td>
          <td>${level.orderCount}</td>
        </tr>
      `;
    })
    .join("");
}

function renderTrades(container, trades, emptyMessage, includeEvent = false) {
  if (trades.length === 0) {
    const colspan = includeEvent ? 5 : 4;
    container.innerHTML = `<tr><td class="empty" colspan="${colspan}">${emptyMessage}</td></tr>`;
    return;
  }

  container.innerHTML = trades
    .map((trade) => {
      if (includeEvent) {
        return `
          <tr>
            <td>${trade.eventTimestamp}</td>
            <td>${trade.buyOrderId}</td>
            <td>${trade.sellOrderId}</td>
            <td>${trade.price}</td>
            <td>${trade.quantity}</td>
          </tr>
        `;
      }
      return `
        <tr>
          <td>${trade.buyOrderId}</td>
          <td>${trade.sellOrderId}</td>
          <td>${trade.price}</td>
          <td>${trade.quantity}</td>
        </tr>
      `;
    })
    .join("");
}

function renderTimeline(metricsTimeline, currentTimestamp) {
  if (metricsTimeline.length === 0) {
    els.timelineChart.innerHTML = "";
    return;
  }

  const width = 600;
  const height = 220;
  const pad = 18;
  const innerWidth = width - pad * 2;
  const innerHeight = height - pad * 2;

  const xValues = metricsTimeline.map((item) => item.eventTimestamp);
  const spreadValues = metricsTimeline.map((item) => item.spread ?? 0);
  const midValues = metricsTimeline.map((item) => item.mid ?? 0);
  const imbalanceValues = metricsTimeline.map((item) => item.depthImbalance ?? 0);

  const minX = Math.min(...xValues);
  const maxX = Math.max(...xValues);
  const minY = Math.min(...spreadValues, ...midValues, ...imbalanceValues);
  const maxY = Math.max(...spreadValues, ...midValues, ...imbalanceValues);
  const safeSpanX = Math.max(1, maxX - minX);
  const safeSpanY = Math.max(1, maxY - minY);

  const toPoint = (x, y) => {
    const px = pad + ((x - minX) / safeSpanX) * innerWidth;
    const py = height - pad - ((y - minY) / safeSpanY) * innerHeight;
    return `${px},${py}`;
  };

  const polyline = (values, color) => `
    <polyline fill="none" stroke="${color}" stroke-width="3" points="${
      metricsTimeline.map((item, index) => toPoint(item.eventTimestamp, values[index])).join(" ")
    }" />
  `;

  const currentX = pad + ((currentTimestamp - minX) / safeSpanX) * innerWidth;
  els.timelineChart.innerHTML = `
    <rect x="0" y="0" width="${width}" height="${height}" fill="rgba(20,69,82,0.03)" />
    ${polyline(spreadValues, getCss("--chart-spread"))}
    ${polyline(midValues, getCss("--chart-mid"))}
    ${polyline(imbalanceValues, getCss("--chart-imbalance"))}
    <line x1="${currentX}" y1="${pad}" x2="${currentX}" y2="${height - pad}" stroke="${getCss("--accent")}" stroke-dasharray="5 5" />
  `;
}

function renderHeatmap(book) {
  const levels = [
    ...book.asks.map((level) => ({ ...level, side: "ASK" })),
    ...book.bids.map((level) => ({ ...level, side: "BID" })),
  ];

  if (levels.length === 0) {
    els.heatmap.innerHTML = `<div class="stack-item"><span>No depth</span><strong>No visible depth to render.</strong></div>`;
    return;
  }

  const maxQty = Math.max(...levels.map((level) => level.quantity), 1);
  els.heatmap.innerHTML = levels
    .map((level) => {
      const intensity = 0.18 + (level.quantity / maxQty) * 0.6;
      const bg = level.side === "BID"
        ? `rgba(73, 160, 120, ${intensity})`
        : `rgba(196, 89, 83, ${intensity})`;
      return `
        <div class="heat-level" style="background:${bg}">
          <span>${level.side} level ${level.levelIndex}</span>
          <strong>${level.price}</strong>
          <span>qty=${level.quantity} orders=${level.orderCount}</span>
        </div>
      `;
    })
    .join("");
}

function renderStrategyPanel() {
  if (!state.strategy) {
    els.strategyPanel.innerHTML = `<div class="metric-item"><span>Strategy data</span><strong>Run ./build/strategy_demo to generate logs/strategy_summary.json</strong></div>`;
    return;
  }

  const items = [
    ["Signals Triggered", state.strategy.signals_triggered],
    ["Orders Sent", state.strategy.orders_sent],
    ["Filled Qty", state.strategy.filled_qty],
    ["Fill Ratio", fixed(state.strategy.fill_ratio)],
    ["Inventory", state.strategy.inventory],
    ["Realized PnL", fixed(state.strategy.realized_pnl)],
    ["Unrealized PnL", valueOrNA(state.strategy.unrealized_pnl)],
    ["Adverse Selection", `${state.strategy.adverse_selection_events} / ${fixed(state.strategy.adverse_selection_notional)}`],
  ];

  els.strategyPanel.innerHTML = items
    .map(([label, value]) => `<div class="metric-item"><span>${label}</span><strong>${value}</strong></div>`)
    .join("");
}

function renderBenchmarkPanel() {
  if (!state.benchmark?.scenarios?.length) {
    els.benchmarkPanel.innerHTML = `<div class="benchmark-card-item">Run <code>make bench</code> to generate benchmark_summary.json.</div>`;
    return;
  }

  els.benchmarkPanel.innerHTML = state.benchmark.scenarios
    .map((scenario) => {
      const maxCount = Math.max(...scenario.latency_histogram.map((bucket) => bucket.count), 1);
      return `
        <div class="benchmark-card-item">
          <strong>${scenario.name}</strong>
          <div class="detail-list" style="margin-top:10px">
            <div class="detail-item"><span>Throughput</span><strong>${Math.round(scenario.throughput).toLocaleString()} ev/s</strong></div>
            <div class="detail-item"><span>Avg Latency</span><strong>${scenario.average_latency_ns} ns</strong></div>
            <div class="detail-item"><span>P99</span><strong>${scenario.p99_latency_ns} ns</strong></div>
            <div class="detail-item"><span>Peak Memory</span><strong>${fixed(scenario.peak_memory_mb)} MB</strong></div>
          </div>
          ${scenario.latency_histogram
            .map((bucket) => `
              <div class="histogram-row">
                <span>${bucket.label}</span>
                <div class="histogram-bar"><div class="histogram-fill" style="width:${(bucket.count / maxCount) * 100}%"></div></div>
                <span>${bucket.count}</span>
              </div>
            `)
            .join("")}
        </div>
      `;
    })
    .join("");
}

function filteredEvents() {
  return state.events.filter((event) => event.symbol === state.selectedSymbol);
}

function snapshotFor(symbol, timestamp) {
  const relevant = state.snapshots.filter(
    (snapshot) => snapshot.symbol === symbol && snapshot.eventTimestamp === timestamp,
  );
  const bids = [];
  const asks = [];
  for (const snapshot of relevant) {
    const item = {
      levelIndex: snapshot.levelIndex,
      price: snapshot.price,
      quantity: snapshot.quantity,
      orderCount: snapshot.orderCount,
    };
    if (snapshot.side === "BID") {
      bids.push(item);
    } else {
      asks.push(item);
    }
  }
  bids.sort((a, b) => a.levelIndex - b.levelIndex);
  asks.sort((a, b) => a.levelIndex - b.levelIndex);
  return { bids, asks };
}

function changedLevelKeys(previousLevels, currentLevels, side) {
  const prev = new Map(previousLevels.map((level) => [level.levelIndex, level]));
  const curr = new Map(currentLevels.map((level) => [level.levelIndex, level]));
  const keys = new Set([...prev.keys(), ...curr.keys()]);
  const changed = new Set();
  for (const key of keys) {
    const a = prev.get(key);
    const b = curr.get(key);
    if (!a || !b || a.price !== b.price || a.quantity !== b.quantity || a.orderCount !== b.orderCount) {
      changed.add(`${side}:${key}`);
    }
  }
  return changed;
}

function getCss(name) {
  return getComputedStyle(document.documentElement).getPropertyValue(name).trim();
}

function fixed(value) {
  return Number(value).toFixed(4);
}

function valueOrNA(value) {
  return value == null ? "NA" : fixed(value);
}

function toNumberOrNull(value) {
  return value === "" || value == null ? null : Number(value);
}

async function fetchText(path) {
  const response = await fetch(path);
  if (!response.ok) {
    throw new Error(`Failed to fetch ${path}: ${response.status}`);
  }
  return response.text();
}

async function fetchOptionalJson(path) {
  const response = await fetch(path);
  if (!response.ok) {
    return null;
  }
  return response.json();
}

function parseCsv(text) {
  const trimmed = text.trim();
  if (!trimmed) {
    return [];
  }
  const lines = trimmed.split(/\r?\n/);
  const headers = lines[0].split(",");
  return lines.slice(1).filter(Boolean).map((line) => {
    const values = line.split(",");
    return headers.reduce((row, header, index) => {
      row[header] = values[index] ?? "";
      return row;
    }, {});
  });
}
