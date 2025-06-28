#ifndef SSCD_SUPERVISEUR_H
#define SSCD_SUPERVISEUR_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <syslog.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/stat.h>

// Structure de configuration
typedef struct
{
    char log_file[256];
    char log_level[10];
    int port;
} Config;

// Déclarations de fonctions
void handle_signal(int signum);
void init_config(void);
void log_event(const char *level, const char *message);
void display_connected_clients(void);
void cleanup_terminated_clients(void);
void add_client(int socket_fd, struct sockaddr_in *client_addr, pid_t pid);
void remove_client(pid_t pid);
void *client_monitor_thread(void *arg);
void *listen_alerts_from_moniteur(void *arg);
void handle_client(int client_socket);

#endif // SSCD_SUPERVISEUR_H
