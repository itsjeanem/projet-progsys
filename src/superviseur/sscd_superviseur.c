#define PORT 8080
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
#include "sscd_superviseur.h"

#define PIPE_PATH "/tmp/moniteur_pipe"
#define MAX_CLIENTS 50

// Structure pour suivre les clients connectés
typedef struct {
    int socket_fd;
    char ip_address[INET_ADDRSTRLEN];
    int port;
    time_t connect_time;
    pid_t process_id;
    int active;
} ClientInfo;

// Variables globales
Config config;
volatile int shutdown_flag = 0;
volatile int running = 1;

// Variables globales pour le suivi des clients
ClientInfo connected_clients[MAX_CLIENTS];
int client_count = 0;
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

// Fonction de gestion des signaux
void handle_signal(int signum)
{
    switch (signum)
    {
    case SIGTERM:
        openlog("Superviseur", LOG_PID | LOG_CONS, LOG_USER);
        syslog(LOG_INFO, "Received SIGTERM: Graceful shutdown initiated");
        closelog();
        shutdown_flag = 1;
        running = 0;
        break;
    case SIGUSR1:
        // Alerte de CPU élevé
        openlog("Superviseur", LOG_PID | LOG_CONS, LOG_USER);
        log_event("ALERTE", "Signal SIGUSR1 reçu : CPU élevé détecté !");
        printf("[Superviseur] 🚨 CPU élevé détecté (SIGUSR1)\n");
        break;
    case SIGUSR2:
        // Rapport d'état des clients connectés
        display_connected_clients();
        break;
    case SIGCHLD:
        // Nettoyage des processus enfants terminés
        cleanup_terminated_clients();
        break;
    }
}

// Initialisation de la configuration
void init_config()
{
    strcpy(config.log_file, "src/superviseur/superviseur.log");
    strcpy(config.log_level, "INFO");
    config.port = PORT;
}

// Journalisation des événements
void log_event(const char *level, const char *message)
{
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char timestamp[20];
    strftime(timestamp, 20, "%Y-%m-%d %H:%M:%S", tm_info);

    FILE *log_file = fopen(config.log_file, "a");
    if (log_file)
    {
        fprintf(log_file, "[%s] [%s] %s\n", timestamp, level, message);
        fclose(log_file);
    }
}

// Ajouter un client à la liste des connectés
void add_client(int socket_fd, struct sockaddr_in *client_addr, pid_t pid)
{
    pthread_mutex_lock(&clients_mutex);
    
    if (client_count < MAX_CLIENTS) {
        ClientInfo *client = &connected_clients[client_count];
        client->socket_fd = socket_fd;
        inet_ntop(AF_INET, &(client_addr->sin_addr), client->ip_address, INET_ADDRSTRLEN);
        client->port = ntohs(client_addr->sin_port);
        client->connect_time = time(NULL);
        client->process_id = pid;
        client->active = 1;
        
        client_count++;
        
        char log_msg[256];
        snprintf(log_msg, sizeof(log_msg), 
                "Nouveau client connecté: %s:%d (PID: %d) - Total: %d clients", 
                client->ip_address, client->port, pid, client_count);
        log_event("INFO", log_msg);
        
        printf("[Superviseur] 📱 Client connecté: %s:%d (PID: %d)\n", 
               client->ip_address, client->port, pid);
    }
    
    pthread_mutex_unlock(&clients_mutex);
}

// Supprimer un client de la liste
void remove_client(pid_t pid)
{
    pthread_mutex_lock(&clients_mutex);
    
    for (int i = 0; i < client_count; i++) {
        if (connected_clients[i].process_id == pid && connected_clients[i].active) {
            connected_clients[i].active = 0;
            
            char log_msg[256];
            snprintf(log_msg, sizeof(log_msg), 
                    "Client déconnecté: %s:%d (PID: %d)", 
                    connected_clients[i].ip_address, 
                    connected_clients[i].port, pid);
            log_event("INFO", log_msg);
            
            printf("[Superviseur] 📱 Client déconnecté: %s:%d (PID: %d)\n", 
                   connected_clients[i].ip_address, 
                   connected_clients[i].port, pid);
            
            // Compacter la liste en supprimant les clients inactifs
            for (int j = i; j < client_count - 1; j++) {
                if (!connected_clients[j].active) {
                    connected_clients[j] = connected_clients[j + 1];
                }
            }
            client_count--;
            break;
        }
    }
    
    pthread_mutex_unlock(&clients_mutex);
}

// Afficher la liste des clients connectés
void display_connected_clients()
{
    pthread_mutex_lock(&clients_mutex);
    
    printf("\n===== CLIENTS CONNECTÉS =====\n");
    printf("Nombre total de clients: %d\n", client_count);
    
    if (client_count == 0) {
        printf("Aucun client connecté.\n");
    } else {
        printf("%-15s %-6s %-10s %-20s\n", "IP", "Port", "PID", "Temps de connexion");
        printf("-------------------------------------------------------\n");
        
        for (int i = 0; i < client_count; i++) {
            if (connected_clients[i].active) {
                time_t duration = time(NULL) - connected_clients[i].connect_time;
                printf("%-15s %-6d %-10d %ld secondes\n",
                       connected_clients[i].ip_address,
                       connected_clients[i].port,
                       connected_clients[i].process_id,
                       duration);
            }
        }
    }
    printf("=============================\n\n");
    
    // Log également l'information
    char log_msg[256];
    snprintf(log_msg, sizeof(log_msg), "État des clients: %d connectés", client_count);
    log_event("INFO", log_msg);
    
    pthread_mutex_unlock(&clients_mutex);
}

// Nettoyer les processus enfants terminés
void cleanup_terminated_clients()
{
    pid_t pid;
    int status;
    
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        remove_client(pid);
    }
}

// Thread pour afficher périodiquement les statistiques
void *client_monitor_thread(void *arg)
{
    (void)arg; // Supprimer le warning sur paramètre inutilisé
    while (running) {
        sleep(30); // Affiche les stats toutes les 30 secondes
        if (running) {
            printf("[Superviseur] 📊 Clients actifs: %d\n", client_count);
        }
    }
    return NULL;
}

// Lecture des alertes du moniteur
void *listen_alerts_from_moniteur(void *arg)
{
    (void)arg; // Supprimer le warning sur paramètre inutilisé
    char buffer[128];
    int fd = open(PIPE_PATH, O_RDONLY);
    if (fd == -1)
    {
        perror("[Superviseur] Erreur ouverture du pipe");
        pthread_exit(NULL);
    }

    printf("[Superviseur] Écoute des alertes du moniteur...\n");

    while (running)
    {
        memset(buffer, 0, sizeof(buffer));
        int n = read(fd, buffer, sizeof(buffer) - 1);
        if (n > 0)
        {
            printf("[ALERTE MONITEUR] %s", buffer);
            log_event("ALERTE", buffer);

            // Détection CPU_HIGH dans l'alerte
            if (strstr(buffer, "CPU_HIGH") != NULL)
            {
                // On déclenche un signal interne
                kill(getpid(), SIGUSR1);
            }
        }
        else
        {
            sleep(1);
        }
    }

    close(fd);
    return NULL;
}

// Gestion des clients TCP (modifiée pour inclure les commandes de gestion des clients)
void handle_client(int client_socket)
{
    char buffer[1024];
    char response[1024];

    sprintf(response, "Superviseur Central SSCD v1.0\r\n> ");
    send(client_socket, response, strlen(response), 0);

    while (1)
    {
        memset(buffer, 0, sizeof(buffer));
        int read_size = recv(client_socket, buffer, sizeof(buffer), 0);
        if (read_size <= 0)
            break;

        buffer[strcspn(buffer, "\r\n")] = 0;

        if (strcmp(buffer, "HELP") == 0)
        {
            strcpy(response, "Available commands: STATUS, CLIENTS, HELP, QUIT\r\n> ");
        }
        else if (strcmp(buffer, "STATUS") == 0)
        {
            snprintf(response, sizeof(response), 
                    "System status: OK - %d clients connectés\r\n> ", client_count);
        }
        else if (strcmp(buffer, "CLIENTS") == 0)
        {
            pthread_mutex_lock(&clients_mutex);
            char client_list[2048] = "Clients connectés:\r\n";
            
            if (client_count == 0) {
                strcat(client_list, "Aucun client connecté.\r\n");
            } else {
                for (int i = 0; i < client_count; i++) {
                    if (connected_clients[i].active) {
                        char client_info[256];
                        time_t duration = time(NULL) - connected_clients[i].connect_time;
                        snprintf(client_info, sizeof(client_info),
                                "- %s:%d (PID: %d, connecté depuis %ld sec)\r\n",
                                connected_clients[i].ip_address,
                                connected_clients[i].port,
                                connected_clients[i].process_id,
                                duration);
                        strcat(client_list, client_info);
                    }
                }
            }
            strcat(client_list, "> ");
            strcpy(response, client_list);
            pthread_mutex_unlock(&clients_mutex);
        }
        else if (strcmp(buffer, "QUIT") == 0)
        {
            strcpy(response, "Goodbye!\r\n");
            send(client_socket, response, strlen(response), 0);
            break;
        }
        else
        {
            strcpy(response, "Unknown command. Type HELP for assistance\r\n> ");
        }
        send(client_socket, response, strlen(response), 0);
    }

    close(client_socket);
}

int main()
{
    int server_fd, client_socket;
    struct sockaddr_in address, client_addr;
    socklen_t client_addrlen = sizeof(client_addr);
    struct sigaction sa;

    init_config();

    // Initialiser la liste des clients
    memset(connected_clients, 0, sizeof(connected_clients));

    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sa.sa_handler = handle_signal;

    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGUSR1, &sa, NULL);
    sigaction(SIGUSR2, &sa, NULL);
    sigaction(SIGCHLD, &sa, NULL);

    // Créer le thread d'écoute d'alerte
    pthread_t alert_thread;
    pthread_create(&alert_thread, NULL, listen_alerts_from_moniteur, NULL);

    // Créer le thread de monitoring des clients
    pthread_t monitor_thread;
    pthread_create(&monitor_thread, NULL, client_monitor_thread, NULL);

    // Création du socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Permettre la réutilisation de l'adresse
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(config.port);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 10) < 0)
    {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Superviseur démarré sur le port %d\n", config.port);
    printf("Commandes disponibles:\n");
    printf("  - SIGUSR2 : Afficher les clients connectés\n");
    printf("  - SIGTERM : Arrêt gracieux\n\n");
    log_event("INFO", "Superviseur démarré avec suivi des clients");

    while (!shutdown_flag)
    {
        client_socket = accept(server_fd, (struct sockaddr *)&client_addr, &client_addrlen);
        if (client_socket < 0)
        {
            if (!shutdown_flag) {
                perror("accept error");
            }
            continue;
        }

        pid_t pid = fork();
        if (pid == 0)
        {
            // Processus enfant
            close(server_fd);
            handle_client(client_socket);
            exit(0);
        }
        else if (pid > 0)
        {
            // Processus parent
            add_client(client_socket, &client_addr, pid);
            close(client_socket);
        }
        else
        {
            perror("fork failed");
            close(client_socket);
        }
    }

    // Nettoyage final
    running = 0;
    pthread_join(alert_thread, NULL);
    pthread_join(monitor_thread, NULL);
    
    // Fermer tous les sockets clients
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < client_count; i++) {
        if (connected_clients[i].active) {
            kill(connected_clients[i].process_id, SIGTERM);
        }
    }
    pthread_mutex_unlock(&clients_mutex);
    
    log_event("INFO", "Arrêt gracieux du superviseur");
    close(server_fd);
    return 0;
}
