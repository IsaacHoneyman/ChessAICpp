# ---- Configuration ---------------------------------------------------------

TARGET   := chess
CXX      := g++
CXXFLAGS := -std=c++20 -O2 -DNDEBUG -Wall -Wextra -Wpedantic
LDFLAGS  :=

# Mode passed to the binary by `make run`. UCI is the default so a bare
# `make run` speaks the protocol a GUI (or the lichess bridge) expects.
# Anything typed after `run` is forwarded to the binary verbatim, so
# `make run hr` and `make run ARGS="hr"` are equivalent.
ARGS     ?= uci

# Words following `run` on the command line are the program's arguments, not
# make goals. Capture them, then give each a do-nothing rule so make does not
# try (and fail) to build a target named after them.
ifeq (run,$(firstword $(MAKECMDGOALS)))
  RUN_ARGS := $(wordlist 2,$(words $(MAKECMDGOALS)),$(MAKECMDGOALS))
  ifneq ($(RUN_ARGS),)
    ARGS := $(RUN_ARGS)
    $(eval $(RUN_ARGS):;@:)
  endif
endif

SRC_DIR   := scripts
BUILD_DIR := build

# Where `make deploy` drops the engine for lichess-bot to launch.
DEPLOY_BIN := $(HOME)/Desktop/C++/lichess-bot/engines/cerabot

# Every scripts/ subdirectory is on the include path, so sources keep using
# flat includes ("board.hpp") regardless of which folder they live in.
INCLUDES := $(addprefix -I,$(sort $(dir $(wildcard $(SRC_DIR)/*/))))
CXXFLAGS += $(INCLUDES)

# The same flags, minus the build-only ones, for clangd (see the `flags` rule).
CLANGD_FLAGS := -std=c++20 -Wall -Wextra -Wpedantic $(INCLUDES)

# Shared engine code: everything under scripts/ except the entry points in
# scripts/app/, which each provide their own main().
MAIN_SRC := $(SRC_DIR)/app/driver.cpp
TEST_SRC := $(SRC_DIR)/app/test.cpp
LIB_SRCS := $(filter-out $(MAIN_SRC) $(TEST_SRC),$(shell find $(SRC_DIR) -name '*.cpp'))

obj = $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(1))

LIB_OBJS  := $(call obj,$(LIB_SRCS))
MAIN_OBJ  := $(call obj,$(MAIN_SRC))
TEST_OBJ  := $(call obj,$(TEST_SRC))
DEPS      := $(LIB_OBJS:.o=.d) $(MAIN_OBJ:.o=.d) $(TEST_OBJ:.o=.d)

BIN      := $(BUILD_DIR)/$(TARGET)
TEST_BIN := $(BUILD_DIR)/test

# ---- Targets ---------------------------------------------------------------

.PHONY: build run deploy test flags clean help

## build: Compile the release build
build: compile_flags.txt $(BIN)

## run: Compile and run the release build; extra words are passed along (make run hr)
run: $(BIN)
	@./$(BIN) $(ARGS)

## deploy: Build, then install the engine into lichess-bot/engines/
deploy: build
	@mkdir -p $(dir $(DEPLOY_BIN))
	@cp $(BIN) $(DEPLOY_BIN)
	@echo "  CP   $(BIN) -> $(DEPLOY_BIN)"

## test: Compile and run the tests (scripts/app/test.cpp)
test: compile_flags.txt $(TEST_BIN)
	@./$(TEST_BIN)

## flags: Regenerate compile_flags.txt so clangd sees every scripts/ folder
flags: compile_flags.txt

## clean: Remove all build artifacts
clean:
	@rm -rf $(BUILD_DIR)
	@echo "Removed $(BUILD_DIR)/"

## help: Show this message
help:
	@echo "Usage: make [target] [args...]"
	@echo "  args after 'run' go straight to the binary, e.g. make run hr"
	@echo
	@sed -n 's/^## \([a-z]*\): \(.*\)/  \1\t\2/p' $(MAKEFILE_LIST) | expand -t 10

# ---- Rules -----------------------------------------------------------------

$(BIN): $(LIB_OBJS) $(MAIN_OBJ)
	@echo "  LD   $@"
	@$(CXX) $(LDFLAGS) $^ -o $@

$(TEST_BIN): $(LIB_OBJS) $(TEST_OBJ)
	@echo "  LD   $@"
	@$(CXX) $(LDFLAGS) $^ -o $@

# scripts/ changes mtime whenever a subdirectory is added or removed, so the
# clangd include path refreshes itself the moment a new folder appears.
compile_flags.txt: $(SRC_DIR) Makefile
	@echo "  GEN  $@"
	@printf '%s\n' $(CLANGD_FLAGS) > $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	@echo "  CXX  $<"
	@$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

-include $(DEPS)
