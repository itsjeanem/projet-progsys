#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <time.h>

#define PIPE_PATH "/tmp/moniteur_pipe"
#define ALERT_CPU_THRESHOLD 75.0
#define LOG_FILE "moniteur_log.csv"

volatile int running = 1;

void handle_signal(int sig) {
    running = 0;
}

// Lecture de la charge CPU depuis /proc/stat
float read_cpu_usage() {
    FILE *fp = fopen("/proc/stat", "r");
    if (!fp) return -1;

    char buffer[1024];
    fgets(buffer, sizeof(buffer), fp); // lecture de la 1re ligne "cpu"
    fclose(fp);

    unsigned long user, nice, system, idle;
    sscanf(buffer, "cpu %lu %lu %lu %lu", &user, &nice, &system, &idle);
    unsigned long total = user + nice + system + idle;
    unsigned long active = user + nice + system;

    static unsigned long prev_total = 0, prev_active = 0;
    float cpu_usage = 0.0;

    if (prev_total != 0) {
        cpu_usage = 100.0 * (active - prev_active) / (total - prev_total);
    }

    prev_total = total;
    prev_active = active;

    return cpu_usage;
}

void log_metrics(float cpu_usage) {
    FILE *fp = fopen(LOG_FILE, "a");
    if (!fp) return;

    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char timestamp[32];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_info);

    fprintf(fp, "%s,%.2f\n", timestamp, cpu_usage);
    fclose(fp);
}

void send_alert(float value) {
    int fd = open(PIPE_PATH, O_WRONLY);
    if (fd >= 0) {
        char alert[128];
        snprintf(alert, sizeof(alert), "ALERT:CPU_HIGH:%.2f\n", value);
        write(fd, alert, strlen(alert));
        close(fd);
    }
}

int main() {
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    // Création du pipe nommé s'il n'existe pas
    mkfifo(PIPE_PATH, 0666);

    printf("[Moniteur] Démarrage de la surveillance...\n");

    while (running) {
        float cpu = read_cpu_usage();
        if (cpu < 0) continue;

        log_metrics(cpu);
        printf("[Moniteur] CPU: %.2f%%\n", cpu);

        if (cpu > ALERT_CPU_THRESHOLD) {
            printf("[Moniteur] 🚨 Alerte: CPU élevé!\n");
            send_alert(cpu);
        }

        sleep(5);
    }

    unlink(PIPE_PATH);
    printf("[Moniteur] Arrêt du moniteur.\n");
    return 0;
}