 #!/bin/bash

# echo "[T=0s] Démarrage du superviseur sur port 8080"
# ./src/superviseur/sscd_superviseur &
# PID_SUPERVISEUR=$!
# sleep 3

echo "[T=1s] Lancement du init pour ordonnanceur"
./src/ordonnanceur/init_processus &
PID_ORDO=$!
sleep 2

echo "[T=1s] Démarrage de l'ordonnanceur (connexion shared memory)"
./src/ordonnanceur/sscd_ordonnanceur &
PID_ORDO=$!
sleep 20



# echo "[T=2s] Démarrage du moniteur (collecte /proc toutes les 5s)"
# ./src/moniteur/sscd_moniteur &
# PID_MONITEUR=$!
# sleep 1

# echo "[T=3s] Démarrage de la base de données (init log)"
# ./src/database/sscd_database &
# PID_DB=$!
# sleep 2

# Attendre ou terminer les processus (optionnel)
# kill $PID_CLIENT1 $PID_CLIENT2 $PID_MONITEUR $PID_DB $PID_ORDO $PID_SUPERVISEUR

echo "Initialisation du système terminée."