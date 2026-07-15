CC      ?= gcc
CFLAGS  ?= -std=c11 -O2 -Wall -Wextra
SRC      = src/emojiseed.c src/sha256.c
BIN      = emojiseed

$(BIN): $(SRC) src/sha256.h
	$(CC) $(CFLAGS) -o $(BIN) $(SRC)

TEST_SHA256 = tests/test_sha256

$(TEST_SHA256): tests/test_sha256.c src/sha256.c src/sha256.h
	$(CC) $(CFLAGS) -o $(TEST_SHA256) tests/test_sha256.c src/sha256.c

# unit tests for sha256
.PHONY: test-unit
test-unit: $(TEST_SHA256)
	./$(TEST_SHA256)

# integration tests that drive the cli binary
.PHONY: test-cli
test-cli: $(BIN)
	sh tests/run_cli.sh

.PHONY: test
test: test-unit test-cli

.PHONY: clean
clean:
	rm -f emojiseed $(TEST_SHA256)
