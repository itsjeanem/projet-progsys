# 🖥️ SSCD – Système de Supervision et de Contrôle Distribué

## 🔍 Description

**SSCD** est un projet de supervision système modulaire réparti, développé en **C sous Linux**.  
Il simule une architecture de gestion distribuée où chaque module joue un rôle dans la surveillance, la planification, le stockage ou l'affichage des ressources système (CPU, RAM, Load Avg, etc).

Le système est composé de modules indépendants, interconnectés via **IPC** (pipes nommés, signaux), **sockets TCP** et **fichiers structurés**. Un **dashboard HTML interactif** permet de visualiser les données.

---

## 🧱 Architecture du projet

```plaintext
.
├── Makefile
├── README.md
├── src/
│   ├── superviseur/      → Superviseur principal TCP + alertes
│   ├── ordonnanceur/     → Planification des tâches (à compléter)
│   ├── gestionnaire/     → Mémoire / messages (à compléter)
│   ├── moniteur/         → Surveillance CPU, RAM, LOAD via /proc
│   ├── database/         → Sauvegarde, rotation, archivage des logs
│   ├── clients/          → Clients simulés TCP
│   └── common/           → Structures partagées (protocol.h, shared_memory.h…)
├── demo/                 → Dashboard HTML + JS
│   ├── index.html
│   ├── data.js
│   └── generate_data.py
```

---

## 🔧 Compilation

Assurez-vous d’avoir les outils suivants :

- `gcc`, `make`
- Un environnement Linux (natif ou via **WSL**)
- `Python 3` pour le dashboard

Puis exécutez :

```bash
make
```

Cela compile les exécutables suivants :

- `sscd_superviseur`
- `sscd_moniteur`
- `sscd_database`
- `sscd_simulateur`
- `sscd_clients`
- `sscd_ordonnanceur`

---

## 🚀 Lancement des modules

```bash
./src/sscd_moniteur
```

> Collecte les métriques CPU, RAM, LOAD toutes les 5 secondes, les écrit dans `moniteur_log.csv` et envoie une alerte CPU>75% via pipe nommé.

---

## 📊 Lancer le dashboard HTML

```bash
cd demo
python3 generate_data.py
python3 -m http.server 8000
```

Puis ouvrir :

```
http://localhost:8000
```

> Le tableau de bord affiche les courbes **CPU**, **RAM**, **LOAD** avec auto-refresh toutes les 30s. Les points rouges indiquent les alertes CPU.

---

## ⚙️ Dépendances

- `Linux` (Ubuntu ou WSL)
- `make`, `gcc`, `gdb`
- `Python 3`
- Navigateur moderne (Chrome, Firefox…)

---
