#ifndef SUPERVISEUR_H
#define SUPERVISEUR_H

#include <pthread.h>

// Configuration système
typedef struct
{
    char log_file[256];
    char log_level[10];
    int port;
} Config;

// Déclaration des variables globales
extern Config config;
extern volatile int shutdown_flag;
extern volatile int running;

// Fonctions publiques
void handle_signal(int signum);
void init_config();
void log_event(const char *level, const char *message);
void *listen_alerts_from_moniteur();
void handle_client(int client_socket);

#endif // SUPERVISEUR_H
