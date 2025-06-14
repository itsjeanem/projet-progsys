CC=gcc
CFLAGS=-Wall -Wextra -g
LDFLAGS=

SRC_SUPERVISEUR=src/superviseur/superviseur.c
SRC_COMMON=src/common/protocol.h

BIN_SUPERVISEUR=sscd_superviseur

all: $(BIN_SUPERVISEUR)

$(BIN_SUPERVISEUR): $(SRC_SUPERVISEUR)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f $(BIN_SUPERVISEUR)

.PHONY: all clean
