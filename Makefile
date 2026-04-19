CXX := clang++
CXXFLAGS := -std=c++20 -O3 -Wall -Wextra -Wpedantic -Iinclude
DEBUG_FLAGS := -std=c++20 -O0 -g -Wall -Wextra -Wpedantic -Iinclude

BUILD_DIR := build
SRC_DIR := src
TEST_DIR := tests
BENCH_DIR := benchmarks

CORE_SOURCES := \
	$(SRC_DIR)/analytics.cpp \
	$(SRC_DIR)/exchange.cpp \
	$(SRC_DIR)/order.cpp \
	$(SRC_DIR)/order_book.cpp \
	$(SRC_DIR)/matching_engine.cpp \
	$(SRC_DIR)/reporting.cpp \
	$(SRC_DIR)/simulator.cpp \
	$(SRC_DIR)/strategy.cpp \
	$(SRC_DIR)/utils.cpp

CORE_OBJECTS := $(CORE_SOURCES:$(SRC_DIR)/%.cpp=$(BUILD_DIR)/%.o)

TEST_TARGETS := \
	$(BUILD_DIR)/test_basic \
	$(BUILD_DIR)/test_matching \
	$(BUILD_DIR)/test_cancel \
	$(BUILD_DIR)/test_partial_fill \
	$(BUILD_DIR)/test_simulator \
	$(BUILD_DIR)/test_analytics \
	$(BUILD_DIR)/test_strategy

.PHONY: all clean test bench run debug

all: $(BUILD_DIR)/demo $(BUILD_DIR)/strategy_demo $(TEST_TARGETS) $(BUILD_DIR)/benchmark

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/demo: $(SRC_DIR)/main.cpp $(CORE_OBJECTS) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(SRC_DIR)/main.cpp $(CORE_OBJECTS) -o $@

$(BUILD_DIR)/strategy_demo: $(SRC_DIR)/strategy_main.cpp $(CORE_OBJECTS) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(SRC_DIR)/strategy_main.cpp $(CORE_OBJECTS) -o $@

$(BUILD_DIR)/benchmark: $(BENCH_DIR)/benchmark.cpp $(CORE_OBJECTS) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(BENCH_DIR)/benchmark.cpp $(CORE_OBJECTS) -o $@

$(BUILD_DIR)/test_basic: $(TEST_DIR)/test_basic.cpp $(CORE_OBJECTS) | $(BUILD_DIR)
	$(CXX) $(DEBUG_FLAGS) $(TEST_DIR)/test_basic.cpp $(CORE_OBJECTS) -o $@

$(BUILD_DIR)/test_matching: $(TEST_DIR)/test_matching.cpp $(CORE_OBJECTS) | $(BUILD_DIR)
	$(CXX) $(DEBUG_FLAGS) $(TEST_DIR)/test_matching.cpp $(CORE_OBJECTS) -o $@

$(BUILD_DIR)/test_cancel: $(TEST_DIR)/test_cancel.cpp $(CORE_OBJECTS) | $(BUILD_DIR)
	$(CXX) $(DEBUG_FLAGS) $(TEST_DIR)/test_cancel.cpp $(CORE_OBJECTS) -o $@

$(BUILD_DIR)/test_partial_fill: $(TEST_DIR)/test_partial_fill.cpp $(CORE_OBJECTS) | $(BUILD_DIR)
	$(CXX) $(DEBUG_FLAGS) $(TEST_DIR)/test_partial_fill.cpp $(CORE_OBJECTS) -o $@

$(BUILD_DIR)/test_simulator: $(TEST_DIR)/test_simulator.cpp $(CORE_OBJECTS) | $(BUILD_DIR)
	$(CXX) $(DEBUG_FLAGS) $(TEST_DIR)/test_simulator.cpp $(CORE_OBJECTS) -o $@

$(BUILD_DIR)/test_analytics: $(TEST_DIR)/test_analytics.cpp $(CORE_OBJECTS) | $(BUILD_DIR)
	$(CXX) $(DEBUG_FLAGS) $(TEST_DIR)/test_analytics.cpp $(CORE_OBJECTS) -o $@

$(BUILD_DIR)/test_strategy: $(TEST_DIR)/test_strategy.cpp $(CORE_OBJECTS) | $(BUILD_DIR)
	$(CXX) $(DEBUG_FLAGS) $(TEST_DIR)/test_strategy.cpp $(CORE_OBJECTS) -o $@

run: $(BUILD_DIR)/demo
	./$(BUILD_DIR)/demo

test: $(TEST_TARGETS)
	./$(BUILD_DIR)/test_basic
	./$(BUILD_DIR)/test_matching
	./$(BUILD_DIR)/test_cancel
	./$(BUILD_DIR)/test_partial_fill
	./$(BUILD_DIR)/test_simulator
	./$(BUILD_DIR)/test_analytics
	./$(BUILD_DIR)/test_strategy

bench: $(BUILD_DIR)/benchmark
	./$(BUILD_DIR)/benchmark

debug: CXXFLAGS := $(DEBUG_FLAGS)
debug: clean all

clean:
	rm -rf $(BUILD_DIR)
