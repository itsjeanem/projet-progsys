CC = gcc
CFLAGS = -Wall -Wextra -g

# Source files
SUPERVISEUR = src/superviseur/sscd_superviseur.c
MONITEUR = src/moniteur/sscd_moniteur.c
DATABASE = src/database/sscd_database.c
SIMULATEUR = src/simulateur/sscd_simulateur.c
CLIENTS = src/clients/sscd_clients.c
ORDONNANCEUR = src/ordonnanceur/init_processus.c src/ordonnanceur/sscd_ordonnanceur.c

# Binary names
BIN_SUPERVISEUR = src/superviseur/sscd_superviseur
BIN_MONITEUR = src/moniteur/sscd_moniteur
BIN_DATABASE = src/database/sscd_database
BIN_SIMULATEUR = src/simulateur/sscd_simulateur
BIN_CLIENTS = src/clients/sscd_clients
BIN_ORDONNANCEUR = src/ordonnanceur/sscd_ordonnanceur


all: $(BIN_SUPERVISEUR) $(BIN_MONITEUR) $(BIN_DATABASE) $(BIN_SIMULATEUR) $(BIN_CLIENTS) $(BIN_ORDONNANCEUR)

$(BIN_SUPERVISEUR): $(SUPERVISEUR)
	$(CC) $(CFLAGS) -o $@ $^

$(BIN_MONITEUR): $(MONITEUR)
	$(CC) $(CFLAGS) -o $@ $^

$(BIN_DATABASE): $(DATABASE)
	$(CC) $(CFLAGS) -o $@ $^

$(BIN_SIMULATEUR) : $(SIMULATEUR)
    $(CC) $(CFLAGS) -o $@ $^

$(BIN_CLIENTS) : $(CLIENTS)
    $(CC) $(CFLAGS) -o $@ $^

$(BIN_ORDONNANCEUR): $(ORDONNANCEUR)
	$(CC) $(CFLAGS) -o $@ $^


clean:
	rm -f $(BIN_SUPERVISEUR) $(BIN_MONITEUR) $(BIN_DATABASE) $(BIN_SIMULATEUR) $(BIN_CLIENTS) $(BIN_ORDONNANCEUR)

.PHONY: all clean
