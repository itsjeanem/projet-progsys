CC = gcc
CFLAGS = -Wall -Wextra -g

# Source files
SUPERVISEUR = src/superviseur/superviseur.c
MONITEUR = src/moniteur/sscd_moniteur.c
DATABASE = src/database/sscd_database.c

# Binary names
BIN_SUPERVISEUR = sscd_superviseur
BIN_MONITEUR = sscd_moniteur
BIN_DATABASE = sscd_database

all: $(BIN_SUPERVISEUR) $(BIN_MONITEUR) $(BIN_DATABASE)

$(BIN_SUPERVISEUR): $(SUPERVISEUR)
	$(CC) $(CFLAGS) -o $@ $^

$(BIN_MONITEUR): $(MONITEUR)
	$(CC) $(CFLAGS) -o $@ $^

$(BIN_DATABASE): $(DATABASE)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f $(BIN_SUPERVISEUR) $(BIN_MONITEUR) $(BIN_DATABASE)

.PHONY: all clean
