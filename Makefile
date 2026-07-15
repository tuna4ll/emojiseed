CC      ?= gcc
CFLAGS  ?= -std=c11 -O2 -Wall -Wextra
SRC      = src/emojiseed.c src/core.c src/sha256.c
BIN      = emojiseed

$(BIN): $(SRC) src/core.h src/sha256.h
	$(CC) $(CFLAGS) -o $(BIN) $(SRC)

TEST_SHA256 = tests/test_sha256
TEST_CORE   = tests/test_core

$(TEST_SHA256): tests/test_sha256.c src/sha256.c src/sha256.h
	$(CC) $(CFLAGS) -o $(TEST_SHA256) tests/test_sha256.c src/sha256.c

$(TEST_CORE): tests/test_core.c src/core.c src/sha256.c src/core.h src/sha256.h
	$(CC) $(CFLAGS) -o $(TEST_CORE) tests/test_core.c src/core.c src/sha256.c

# unit tests for sha256 and the core encoder
.PHONY: test-unit
test-unit: $(TEST_SHA256) $(TEST_CORE)
	./$(TEST_SHA256)
	./$(TEST_CORE)

# integration tests that drive the cli binary
.PHONY: test-cli
test-cli: $(BIN)
	sh tests/run_cli.sh

.PHONY: test
test: test-unit test-cli

# webassembly build of the core for the browser demo
.PHONY: wasm
wasm:
	sh web/build.sh

.PHONY: clean
clean:
	rm -f emojiseed $(TEST_SHA256) $(TEST_CORE)
