#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>

#define SUPERVISEUR_IP "127.0.0.1"
#define SUPERVISEUR_PORT 8080
#define REPORT_INTERVAL 5

volatile int running = 1;

void handle_signal(int sig) {
    if (sig == SIGTERM || sig == SIGINT) {
        printf("Worker shutting down gracefully...\n");
        running = 0;
    }
}

// Simule des tâches (calculs ou I/O)
void* simulate_task(void* arg) {
    int id = *(int*)arg;
    printf("Tâche %d démarrée.\n", id);
    sleep(rand() % 3 + 2); // simulation de tâche aléatoire
    printf("Tâche %d terminée.\n", id);
    return NULL;
}

// Envoie un état périodique au superviseur
void* report_status(void* sockfd_ptr) {
    int sockfd = *(int*)sockfd_ptr;
    char buffer[256];
    while (running) {
        int cpu_load = rand() % 100;
        int mem_use = rand() % 100;
        sprintf(buffer, "STATUS CPU:%d%% MEM:%d%%\n", cpu_load, mem_use);
        send(sockfd, buffer, strlen(buffer), 0);
        sleep(REPORT_INTERVAL);
    }
    return NULL;
}

int main() {
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);
    srand(time(NULL));

    int sock;
    struct sockaddr_in server;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("socket");
        exit(1);
    }

    server.sin_addr.s_addr = inet_addr(SUPERVISEUR_IP);
    server.sin_family = AF_INET;
    server.sin_port = htons(SUPERVISEUR_PORT);

    if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
        perror("connect");
        close(sock);
        exit(1);
    }

    printf("Connecté au superviseur.\n");

    // Lancer le thread de reporting
    pthread_t reporter_thread;
    pthread_create(&reporter_thread, NULL, report_status, &sock);

    // Lancer des tâches simulées
    pthread_t task_threads[5];
    int task_ids[5];
    for (int i = 0; i < 5 && running; i++) {
        task_ids[i] = i + 1;
        pthread_create(&task_threads[i], NULL, simulate_task, &task_ids[i]);
        sleep(1); // pour espacer les démarrages
    }

    for (int i = 0; i < 5; i++) {
        pthread_join(task_threads[i], NULL);
    }

    running = 0;
    pthread_join(reporter_thread, NULL);

    close(sock);
    printf("Déconnexion du superviseur.\n");

    return 0;
}
