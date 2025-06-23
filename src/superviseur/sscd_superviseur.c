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

#define PIPE_PATH "/tmp/moniteur_pipe"

// Structure de configuration
typedef struct
{
    char log_file[256];
    char log_level[10];
    int port;
} Config;

Config config;
volatile int shutdown_flag = 0;
volatile int running = 1;

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
        // Rapport d'état
        break;
    }
}

// Initialisation de la configuration
void init_config()
{
    strcpy(config.log_file, "superviseur.log");
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

// Lecture des alertes du moniteur
void *listen_alerts_from_moniteur(void *arg)
{
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

// Gestion des clients TCP
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
            strcpy(response, "Available commands: STATUS, HELP, QUIT\r\n> ");
        }
        else if (strcmp(buffer, "STATUS") == 0)
        {
            strcpy(response, "System status: OK\r\n> ");
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
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    struct sigaction sa;

    init_config();

    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sa.sa_handler = handle_signal;

    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGUSR1, &sa, NULL);
    sigaction(SIGUSR2, &sa, NULL);

    // Créer le thread d'écoute d'alerte
    pthread_t alert_thread;
    pthread_create(&alert_thread, NULL, listen_alerts_from_moniteur, NULL);

    // Création du socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

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

    printf("Superviseur running on port %d\n", config.port);
    log_event("INFO", "Superviseur démarré");

    while (!shutdown_flag)
    {
        client_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
        if (client_socket < 0)
        {
            perror("accept error");
            continue;
        }

        log_event("INFO", "Nouvelle connexion client");

        pid_t pid = fork();
        if (pid == 0)
        {
            close(server_fd);
            handle_client(client_socket);
            exit(0);
        }
        else if (pid > 0)
        {
            close(client_socket);
        }
        else
        {
            perror("fork failed");
        }
    }

    running = 0;
    pthread_join(alert_thread, NULL);
    log_event("INFO", "Arrêt gracieux du superviseur");
    close(server_fd);
    return 0;
}