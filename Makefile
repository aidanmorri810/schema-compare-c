# Makefile for schema-compare-c

# Compiler and flags
CC = gcc

# Use pg_config to locate PostgreSQL libraries and headers for cross-platform compatibility
PG_CONFIG_CFLAGS := $(shell pg_config --cflags)
PG_CONFIG_LDFLAGS := $(shell pg_config --ldflags)
PG_LIBDIR := $(shell pg_config --libdir)
PG_INCLUDEDIR := $(shell pg_config --includedir)

# Extract only include paths and essential flags, filter out warning flags from pg_config
PG_CFLAGS := $(filter -I% -D% -f%,$(PG_CONFIG_CFLAGS))
# Ensure include directory is present
ifeq ($(findstring -I$(PG_INCLUDEDIR),$(PG_CFLAGS)),)
PG_CFLAGS += -I$(PG_INCLUDEDIR)
endif

# Ensure library directory is in linker flags
PG_LDFLAGS := $(PG_CONFIG_LDFLAGS)
ifeq ($(findstring $(PG_LIBDIR),$(PG_LDFLAGS)),)
PG_LDFLAGS += -L$(PG_LIBDIR)
endif

CFLAGS = -Wall -Wextra -Werror -std=c11 -Iinclude -D_POSIX_C_SOURCE=200809L $(PG_CFLAGS)
LDFLAGS = $(PG_LDFLAGS) -lpq
DEBUG_FLAGS = -g -O0 -DDEBUG
RELEASE_FLAGS = -Oz -DNDEBUG -flto -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-unwind-tables

# Platform-specific linker flags
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
    RELEASE_LDFLAGS = -flto -Wl,-dead_strip,-s
else
    RELEASE_LDFLAGS = -s -flto -Wl,--gc-sections,--hash-style=gnu,--as-needed
endif

# Directories
SRC_DIR = src
INC_DIR = include
BUILD_DIR = build
BIN_DIR = bin
TEST_DIR = tests
OBJ_DIR = $(BUILD_DIR)/obj
DEP_DIR = $(BUILD_DIR)/deps

# Target executable
TARGET = $(BIN_DIR)/schema-compare
TEST_TARGET = $(BIN_DIR)/test-runner

# Source files organized by module
PARSER_SRC = $(wildcard $(SRC_DIR)/parser/*.c)
DB_READER_SRC = $(wildcard $(SRC_DIR)/db_reader/*.c)
MEMORY_SRC = $(wildcard $(SRC_DIR)/memory/*.c)
COMPARE_SRC = $(wildcard $(SRC_DIR)/compare/*.c)
OUTPUT_SRC = $(wildcard $(SRC_DIR)/output/*.c)
UTILS_SRC = $(wildcard $(SRC_DIR)/utils/*.c)
MAIN_SRC = $(SRC_DIR)/main.c

# All source files (excluding main for now since it doesn't exist yet)
LIB_SRC = $(PARSER_SRC) $(DB_READER_SRC) $(MEMORY_SRC) $(COMPARE_SRC) \
          $(OUTPUT_SRC) $(UTILS_SRC)

ALL_SRC = $(LIB_SRC) $(MAIN_SRC)

# Object files
LIB_OBJS = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(LIB_SRC))
MAIN_OBJ = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(MAIN_SRC))
OBJS = $(LIB_OBJS) $(MAIN_OBJ)

# Test files
TEST_FRAMEWORK_SRC = $(TEST_DIR)/test_framework.c
TEST_RUNNER_SRC = $(TEST_DIR)/test_runner.c
TEST_UNIT_SRC = $(wildcard $(TEST_DIR)/unit/*.c)
TEST_INTEGRATION_SRC = $(wildcard $(TEST_DIR)/integration/*.c)

TEST_ALL_SRC = $(TEST_FRAMEWORK_SRC) $(TEST_RUNNER_SRC) $(TEST_UNIT_SRC) $(TEST_INTEGRATION_SRC)
TEST_OBJS = $(patsubst $(TEST_DIR)/%.c,$(OBJ_DIR)/test/%.o,$(TEST_ALL_SRC))

# Default target
all: dirs $(TARGET)

# Release build
release: CFLAGS += $(RELEASE_FLAGS)
# Append release-specific linker flags rather than overwriting LDFLAGS so we keep $(PG_LDFLAGS) and -lpq
release: LDFLAGS += $(RELEASE_LDFLAGS)
release: clean $(TARGET)

# Debug build
debug: CFLAGS += $(DEBUG_FLAGS)
debug: clean $(TARGET)

# Create directories
dirs:
	@mkdir -p $(OBJ_DIR)/parser
	@mkdir -p $(OBJ_DIR)/db_reader
	@mkdir -p $(OBJ_DIR)/memory
	@mkdir -p $(OBJ_DIR)/compare
	@mkdir -p $(OBJ_DIR)/output
	@mkdir -p $(OBJ_DIR)/utils
	@mkdir -p $(OBJ_DIR)/test
	@mkdir -p $(OBJ_DIR)/test/unit
	@mkdir -p $(OBJ_DIR)/test/integration
	@mkdir -p $(BIN_DIR)
	@mkdir -p $(DEP_DIR)

# Compile source files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	@mkdir -p $(DEP_DIR)
	$(CC) $(CFLAGS) -MMD -MP -MF $(DEP_DIR)/$(notdir $*).d -c $< -o $@

# Compile test files
$(OBJ_DIR)/test/%.o: $(TEST_DIR)/%.c
	@mkdir -p $(dir $@)
	@mkdir -p $(DEP_DIR)
	$(CC) $(CFLAGS) -I$(TEST_DIR) -MMD -MP -MF $(DEP_DIR)/test_$(notdir $*).d -c $< -o $@

# Link main executable (only if main.c exists)
$(TARGET): $(OBJS)
	@mkdir -p $(BIN_DIR)
	@if [ -f $(MAIN_SRC) ]; then \
		$(CC) $(OBJS) $(LDFLAGS) -o $@; \
		echo "Built $(TARGET)"; \
	else \
		echo "Skipping $(TARGET) - main.c not yet created"; \
	fi

# Link test executable
$(TEST_TARGET): $(TEST_OBJS) $(LIB_OBJS)
	@mkdir -p $(BIN_DIR)
	$(CC) $(TEST_OBJS) $(LIB_OBJS) $(LDFLAGS) -o $@
	@echo "Built $(TEST_TARGET)"

# Build library only (useful during development)
lib: dirs $(LIB_OBJS)
	@echo "Built library objects"

# Run tests
test: $(TEST_TARGET)
	$(TEST_TARGET)

# Database test targets
.PHONY: db-up db-down db-reset db-logs db-shell test-db test-all

db-up:
	@echo "Starting PostgreSQL test database..."
	@docker-compose up -d postgres-test 2>&1 | grep -v "attribute.*version.*obsolete" || true
	@echo "Waiting for database to be ready..."
	@for i in 1 2 3 4 5 6 7 8 9 10; do \
		if docker-compose exec -T postgres-test pg_isready -U testuser -d schema_compare_test >/dev/null 2>&1; then \
			echo "Database is ready!"; \
			exit 0; \
		fi; \
		sleep 1; \
	done; \
	echo "Database ready check complete"

db-down:
	@echo "Stopping PostgreSQL test database..."
	docker-compose down

db-reset:
	@echo "Resetting PostgreSQL test database..."
	@docker-compose down -v 2>&1 | grep -v "attribute.*version.*obsolete" || true
	@docker-compose up -d postgres-test 2>&1 | grep -v "attribute.*version.*obsolete" || true
	@echo "Waiting for database to be ready..."
	@for i in 1 2 3 4 5 6 7 8 9 10; do \
		if docker-compose exec -T postgres-test pg_isready -U testuser -d schema_compare_test >/dev/null 2>&1; then \
			echo "Database reset complete!"; \
			exit 0; \
		fi; \
		sleep 1; \
	done; \
	echo "Database reset complete!"

db-logs:
	docker-compose logs -f postgres-test

db-shell:
	docker-compose exec postgres-test psql -U testuser -d schema_compare_test

test-db: db-up $(TEST_TARGET)
	@echo "Running database integration tests..."
	@RUN_DB_TESTS=1 PGHOST=localhost PGPORT=5433 PGDATABASE=schema_compare_test \
	PGUSER=testuser PGPASSWORD=testpass $(TEST_TARGET)

test-all: test-db

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)

# Install
install: $(TARGET)
	install -m 755 $(TARGET) /usr/local/bin/

# Uninstall
uninstall:
	rm -f /usr/local/bin/schema-compare

# Generate tags for development
tags:
	ctags -R $(SRC_DIR) $(INC_DIR)

# Show variables (for debugging makefile)
show:
	@echo "LIB_SRC: $(LIB_SRC)"
	@echo "LIB_OBJS: $(LIB_OBJS)"
	@echo "CFLAGS: $(CFLAGS)"

# Include dependency files
-include $(wildcard $(DEP_DIR)/*.d)

.PHONY: all release debug dirs lib test clean install uninstall tags show
