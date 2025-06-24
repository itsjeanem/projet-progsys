#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <time.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include "sscd_moniteur.h"

#define PIPE_PATH "/tmp/moniteur_pipe"
#define ALERT_CPU_THRESHOLD 75.0
#define LOG_FILE "src/moniteur/moniteur_log.csv"

volatile int running = 1;

void handle_signal()
{
    running = 0;
}

// Lecture de la charge CPU depuis /proc/stat
float read_cpu_usage()
{
    FILE *fp = fopen("/proc/stat", "r");
    if (!fp)
        return -1;

    char buffer[1024];
    fgets(buffer, sizeof(buffer), fp); // lecture de la 1re ligne "cpu"
    fclose(fp);

    unsigned long user, nice, system, idle;
    sscanf(buffer, "cpu %lu %lu %lu %lu", &user, &nice, &system, &idle);
    unsigned long total = user + nice + system + idle;
    unsigned long active = user + nice + system;

    static unsigned long prev_total = 0, prev_active = 0;
    float cpu_usage = 0.0;

    if (prev_total != 0)
    {
        cpu_usage = 100.0 * (active - prev_active) / (total - prev_total);
    }

    prev_total = total;
    prev_active = active;

    return cpu_usage;
}

float read_ram_usage()
{
    FILE *fp = fopen("/proc/meminfo", "r");
    if (!fp)
        return -1;

    char label[64];
    unsigned long total = 0, free = 0, buffers = 0, cached = 0;

    while (!feof(fp))
    {
        fscanf(fp, "%63s %lu kB\n", label, &total);

        if (strcmp(label, "MemTotal:") == 0)
        {
            fscanf(fp, "%lu", &total);
        }
        else if (strcmp(label, "MemAvailable:") == 0)
        {
            fscanf(fp, "%lu", &free);
        }
        else if (strcmp(label, "Buffers:") == 0)
        {
            fscanf(fp, "%lu", &buffers);
        }
        else if (strcmp(label, "Cached:") == 0)
        {
            fscanf(fp, "%lu", &cached);
        }

        if (total && free)
            break;
    }

    fclose(fp);

    if (total == 0)
        return -1;
    float used = 100.0 * (1.0 - ((float)free / total));
    return used;
}

float read_load_average()
{
    FILE *fp = fopen("/proc/loadavg", "r");
    if (!fp)
        return -1;

    float load = 0.0;
    fscanf(fp, "%f", &load);
    fclose(fp);

    return load;
}

void log_metrics(float cpu, float ram, float load)
{
    FILE *fp = fopen(LOG_FILE, "a");
    if (!fp)
        return;

    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char timestamp[32];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_info);

    // Format étendu : 3 lignes par mesure
    fprintf(fp, "%s,METRIC,CPU,%.2f\n", timestamp, cpu);
    fprintf(fp, "%s,METRIC,RAM,%.2f\n", timestamp, ram);
    fprintf(fp, "%s,METRIC,LOAD,%.2f\n", timestamp, load);
    fclose(fp);
}

void send_alert(float value)
{
    int fd = open(PIPE_PATH, O_WRONLY);
    if (fd >= 0)
    {
        char alert[128];
        snprintf(alert, sizeof(alert), "ALERT:CPU_HIGH:%.2f\n", value);
        write(fd, alert, strlen(alert));
        close(fd);
    }
}

int main()
{
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    // Création du pipe nommé s'il n'existe pas
    mkfifo(PIPE_PATH, 0666);

    printf("[Moniteur] Démarrage de la surveillance...\n");

    while (running)
    {
        float cpu = read_cpu_usage();
        float ram = read_ram_usage();
        float load = read_load_average();

        if (cpu < 0 || ram < 0 || load < 0)
            continue;

        log_metrics(cpu, ram, load);

        printf("[Moniteur] CPU: %.2f%% | RAM: %.2f%% | LOAD: %.2f\n", cpu, ram, load);

        if (cpu > ALERT_CPU_THRESHOLD)
        {
            printf("[Moniteur] 🚨 Alerte: CPU élevé!\n");
            send_alert(cpu);
        }

        sleep(5); // Pause de 5 secondes entre les mesures
    }

    unlink(PIPE_PATH);
    printf("[Moniteur] Arrêt du moniteur.\n");
    return 0;
}