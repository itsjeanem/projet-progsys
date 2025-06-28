CC = gcc
CFLAGS = -Wall -Wextra -g

# Source files
SUPERVISEUR = src/superviseur/sscd_superviseur.c
MONITEUR = src/moniteur/sscd_moniteur.c
DATABASE = src/database/sscd_database.c
SIMULATEUR = src/simulateur/sscd_simulateur.c
CLIENTS = src/clients/sscd_clients.c
INIT_PROCESSUS = src/ordonnanceur/init_processus.c
ORDONNANCEUR = src/ordonnanceur/sscd_ordonnanceur.c

# Binary names
BIN_SUPERVISEUR = src/superviseur/sscd_superviseur
BIN_MONITEUR = src/moniteur/sscd_moniteur
BIN_DATABASE = src/database/sscd_database
BIN_SIMULATEUR = src/simulateur/sscd_simulateur
BIN_CLIENTS = src/clients/sscd_clients
BIN_INIT_PROCESSUS = src/ordonnanceur/init_processus
BIN_ORDONNANCEUR = src/ordonnanceur/sscd_ordonnanceur

# Target principal - compile tous les binaires
all: $(BIN_SUPERVISEUR) $(BIN_MONITEUR) $(BIN_DATABASE) $(BIN_SIMULATEUR) $(BIN_CLIENTS) $(BIN_INIT_PROCESSUS) $(BIN_ORDONNANCEUR)

# Règles de compilation individuelles
$(BIN_SUPERVISEUR): $(SUPERVISEUR)
	$(CC) $(CFLAGS) -o $@ $^

$(BIN_MONITEUR): $(MONITEUR)
	$(CC) $(CFLAGS) -o $@ $^

$(BIN_DATABASE): $(DATABASE)
	$(CC) $(CFLAGS) -o $@ $^

$(BIN_SIMULATEUR): $(SIMULATEUR)
	$(CC) $(CFLAGS) -o $@ $^

$(BIN_CLIENTS): $(CLIENTS)
	$(CC) $(CFLAGS) -o $@ $^

# Séparation des deux programmes de l'ordonnanceur
$(BIN_INIT_PROCESSUS): $(INIT_PROCESSUS)
	$(CC) $(CFLAGS) -o $@ $^

$(BIN_ORDONNANCEUR): $(ORDONNANCEUR)
	$(CC) $(CFLAGS) -o $@ $^

# Règles spéciales pour l'ordonnanceur
ordonnanceur: $(BIN_INIT_PROCESSUS) $(BIN_ORDONNANCEUR)
	@echo "=== Programmes de l'ordonnanceur compilés ==="
	@echo "1. Lancez d'abord: ./$(BIN_INIT_PROCESSUS)"
	@echo "2. Puis lancez: ./$(BIN_ORDONNANCEUR)"

# Nettoyage
clean:
	rm -f $(BIN_SUPERVISEUR) $(BIN_MONITEUR) $(BIN_DATABASE) $(BIN_SIMULATEUR) $(BIN_CLIENTS) $(BIN_INIT_PROCESSUS) $(BIN_ORDONNANCEUR)

# Nettoyage de la mémoire partagée (utile pour les tests)
clean-shm:
	@echo "Nettoyage de la mémoire partagée..."
	@ipcs -m | grep $(shell printf "0x%x" 0x1234) | awk '{print $$2}' | xargs -r ipcrm -m || true

# Aide
help:
	@echo "Targets disponibles:"
	@echo "  all          - Compile tous les programmes"
	@echo "  ordonnanceur - Compile seulement les programmes de l'ordonnanceur"
	@echo "  clean        - Supprime tous les binaires"
	@echo "  clean-shm    - Nettoie la mémoire partagée"
	@echo "  help         - Affiche cette aide"
	@echo ""
	@echo "Pour tester l'ordonnanceur:"
	@echo "  1. make ordonnanceur"
	@echo "  2. ./src/ordonnanceur/init_processus"
	@echo "  3. ./src/ordonnanceur/sscd_ordonnanceur"

.PHONY: all clean clean-shm help ordonnanceur