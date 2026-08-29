# ---- Configuration ---------------------------------------------------------

TARGET   := chess
CXX      := g++
CXXFLAGS := -std=c++20 -Wall -Wextra -Wpedantic
LDFLAGS  :=

# Build configuration: release (default) or debug.  Override on the command
# line, e.g. `make BUILD=debug run`.
BUILD    ?= release

# Sanitizers in debug builds.  Disable with `make debug SAN=0` when you want
# full speed (asan costs roughly 2x).
SAN      ?= 1

# Suffix keeps sanitized and unsanitized objects in separate trees, so
# toggling SAN rebuilds correctly instead of reusing stale objects.
VARIANT  := $(BUILD)

ifeq ($(BUILD),debug)
    # Optimized like release, but asserts stay live (no -DNDEBUG) and we keep
    # full debug info.  -fno-omit-frame-pointer keeps stack traces readable.
    CXXFLAGS += -O2 -g3 -fno-omit-frame-pointer -DDEBUG
    ifneq ($(SAN),0)
        CXXFLAGS += -fsanitize=address,undefined
        LDFLAGS  += -fsanitize=address,undefined
    else
        VARIANT := debug-nosan
    endif
else ifeq ($(BUILD),release)
    CXXFLAGS += -O2 -DNDEBUG
else
    $(error Unknown BUILD '$(BUILD)'; expected 'debug' or 'release')
endif

BUILD_DIR := build/$(VARIANT)
SRCS      := $(filter-out test.cpp,$(wildcard *.cpp))
OBJS      := $(SRCS:%.cpp=$(BUILD_DIR)/%.o)
DEPS      := $(OBJS:.o=.d)
BIN       := $(BUILD_DIR)/$(TARGET)
TEST_BIN  := $(BUILD_DIR)/test
TEST_SRCS := board.cpp movegen.cpp zobrist.cpp test.cpp
TEST_OBJS := $(TEST_SRCS:%.cpp=$(BUILD_DIR)/%.test.o)
TEST_DEPS := $(TEST_OBJS:.o=.d)

# Arguments forwarded to the program by `make run ARGS="..."`.
ARGS ?=

# Pin the test binary to one fixed core so timings are comparable between runs
# (no migration between cores, warm caches).  Override with `make test TEST_CPU=n`;
# TEST_CPU= (empty) disables pinning, as does a system without taskset.
TEST_CPU ?= 2
TASKSET  := $(if $(and $(TEST_CPU),$(shell command -v taskset 2>/dev/null)),taskset -c $(TEST_CPU))

# ---- Targets ---------------------------------------------------------------

.PHONY: all build run test debug release clean help

all: build

## build: Compile the project (BUILD=debug|release)
build: $(BIN)

## run: Build, then run the program (pass ARGS="...")
run: $(BIN)
	@./$(BIN) $(ARGS)

## test: Build and run test.cpp (pinned to core TEST_CPU)
test: $(TEST_BIN)
	@$(TASKSET) ./$(TEST_BIN)

## debug: Build and run at -O2 with asserts, debug info, sanitizers (SAN=0 to skip)
debug:
	@$(MAKE) --no-print-directory BUILD=debug run

## release: Build and run optimized (-O2)
release:
	@$(MAKE) --no-print-directory BUILD=release run

## clean: Remove all build artifacts
clean:
	@rm -rf build
	@echo "Removed build/"

## help: Show this message
help:
	@echo "Usage: make [target] [BUILD=debug|release] [SAN=0|1] [TEST_CPU=n] [ARGS=\"...\"]"
	@echo
	@echo "Targets:"
	@sed -n 's/^## \([a-z]*\): \(.*\)/  \1\t\2/p' $(MAKEFILE_LIST) | expand -t 12
	@echo
	@echo "Current: BUILD=$(BUILD)  SAN=$(SAN)  TEST_CPU=$(TEST_CPU)  ->  $(BIN)"

# ---- Rules -----------------------------------------------------------------

$(BIN): $(OBJS)
	@echo "  LD   $@"
	@$(CXX) $(LDFLAGS) $^ -o $@

$(TEST_BIN): $(TEST_OBJS)
	@echo "  LD   $@"
	@$(CXX) $(LDFLAGS) $^ -o $@

$(BUILD_DIR)/%.o: %.cpp | $(BUILD_DIR)
	@echo "  CXX  $<"
	@$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

$(BUILD_DIR)/%.test.o: %.cpp | $(BUILD_DIR)
	@echo "  CXX  $<"
	@$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

$(BUILD_DIR):
	@mkdir -p $@

-include $(DEPS)
-include $(TEST_DEPS)
