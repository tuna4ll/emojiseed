CC      ?= gcc
CFLAGS  ?= -std=c11 -O2 -Wall -Wextra
SRC      = src/emojiseed.c src/sha256.c
BIN      = emojiseed

$(BIN): $(SRC) src/sha256.h
	$(CC) $(CFLAGS) -o $(BIN) $(SRC)

.PHONY: clean
clean:
	rm -f emojiseed
