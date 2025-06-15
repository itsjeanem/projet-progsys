CC=gcc
CFLAGS=-Wall -Wextra -g

# Modules
SUPERVISEUR=src/superviseur/superviseur.c
COMMON=src/common/protocol.h src/common/shared_memory.h src/common/memory_message.h

BIN_SUPERVISEUR=sscd_superviseur

all: $(BIN_SUPERVISEUR)

$(BIN_SUPERVISEUR): $(SUPERVISEUR)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f $(BIN_SUPERVISEUR)

.PHONY: all clean
