 #!/bin/bash


echo "[T=1s] Connexion du Client Worker #1"
./src/clients/sscd_clients 192.168.1.10 45231 &
PID_CLIENT1=$!
sleep 1

echo "[T=2s] Connexion du Client Worker #2"
./src/clients/sscd_clients 192.168.1.11 45232 &
PID_CLIENT2=$!
sleep 20



echo "création des clients treminée."