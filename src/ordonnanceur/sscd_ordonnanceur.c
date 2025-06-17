#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <string.h>
#include <time.h>

#define MAX_PROCESSES 10
#define TIME_QUANTUM 2
#define SHM_KEY 0x1234  // Clé utilisée pour la mémoire partagée

typedef enum {READY, RUNNING, WAITING, TERMINATED} ProcessState;

typedef struct {
    int pid;
    int arrival_time;
    int burst_time;
    int priority;
    ProcessState state;
    int remaining_time;
} Process;

typedef struct {
    int algo; // 0: FIFO, 1: Round Robin, 2: Priority
    int process_count;
    Process processes[MAX_PROCESSES];
} SharedData;

// Fonction d’affichage d’état
const char* get_state_string(ProcessState state) {
    switch (state) {
        case READY: return "READY";
        case RUNNING: return "RUNNING";
        case WAITING: return "WAITING";
        case TERMINATED: return "TERMINATED";
        default: return "UNKNOWN";
    }
}

// Affiche les métriques finales
void afficher_stats(int wt_total, int tt_total, int n, double start_time, double end_time) {
    if (n == 0) return;
    double avg_wt = (double)wt_total / n;
    double avg_tt = (double)tt_total / n;
    double throughput = (double)n / (end_time - start_time);

    printf("\n--- Résultats ---\n");
    printf("Temps moyen d’attente    : %.2f\n", avg_wt);
    printf("Temps moyen de rotation  : %.2f\n", avg_tt);
    printf("Débit (throughput)       : %.2f processus/sec\n", throughput);
    printf("--------------------------\n\n");
}

// FIFO
void ordonnance_fifo(SharedData *shared) {
    printf("\n--- Ordonnancement: FIFO ---\n");
    int current_time = 0;
    int wt_total = 0, tt_total = 0;
    int completed = 0;
    double start = time(NULL);

    for (int i = 0; i < shared->process_count; i++) {
        Process *p = &shared->processes[i];
        if (current_time < p->arrival_time)
            current_time = p->arrival_time;

        p->state = RUNNING;
        printf("[PID=%d] État=%s | Exécution pendant %d unités\n", p->pid, get_state_string(p->state), p->burst_time);
        sleep(p->burst_time); // Simulation

        current_time += p->burst_time;
        int turnaround = current_time - p->arrival_time;
        int wait = turnaround - p->burst_time;
        wt_total += wait;
        tt_total += turnaround;
        p->state = TERMINATED;
        completed++;
    }

    double end = time(NULL);
    afficher_stats(wt_total, tt_total, completed, start, end);
}

// Round Robin
void ordonnance_rr(SharedData *shared) {
    printf("\n--- Ordonnancement: Round Robin ---\n");
    int current_time = 0;
    int wt_total = 0, tt_total = 0;
    int completed = 0;
    int n = shared->process_count;
    double start = time(NULL);

    while (completed < n) {
        int all_done = 1;

        for (int i = 0; i < n; i++) {
            Process *p = &shared->processes[i];
            if (p->state == TERMINATED || p->arrival_time > current_time)
                continue;

            all_done = 0;

            if (p->remaining_time > 0) {
                int exec_time = (p->remaining_time > TIME_QUANTUM) ? TIME_QUANTUM : p->remaining_time;
                p->state = RUNNING;
                printf("[PID=%d] RUNNING pendant %d unités\n", p->pid, exec_time);
                sleep(exec_time); // Simulation
                current_time += exec_time;
                p->remaining_time -= exec_time;

                if (p->remaining_time == 0) {
                    p->state = TERMINATED;
                    int turnaround = current_time - p->arrival_time;
                    int wait = turnaround - p->burst_time;
                    wt_total += wait;
                    tt_total += turnaround;
                    completed++;
                } else {
                    p->state = READY;
                }
            }
        }

        if (all_done)
            break;
    }

    double end = time(NULL);
    afficher_stats(wt_total, tt_total, completed, start, end);
}

// Priorité
void ordonnance_priorite(SharedData *shared) {
    printf("\n--- Ordonnancement: Priorité ---\n");
    int current_time = 0;
    int wt_total = 0, tt_total = 0;
    int completed = 0;
    int n = shared->process_count;
    double start = time(NULL);

    int done[MAX_PROCESSES] = {0};

    while (completed < n) {
        int idx = -1;
        int min_prio = 999;

        for (int i = 0; i < n; i++) {
            Process *p = &shared->processes[i];
            if (!done[i] && p->arrival_time <= current_time && p->priority < min_prio) {
                idx = i;
                min_prio = p->priority;
            }
        }

        if (idx == -1) {
            current_time++;
            continue;
        }

        Process *p = &shared->processes[idx];
        p->state = RUNNING;
        printf("[PID=%d] PRIORITY=%d | RUNNING %d unités\n", p->pid, p->priority, p->burst_time);
        sleep(p->burst_time); // Simulation

        current_time += p->burst_time;
        int turnaround = current_time - p->arrival_time;
        int wait = turnaround - p->burst_time;
        wt_total += wait;
        tt_total += turnaround;
        p->state = TERMINATED;
        done[idx] = 1;
        completed++;
    }

    double end = time(NULL);
    afficher_stats(wt_total, tt_total, completed, start, end);
}

int main() {
    int shmid = shmget(SHM_KEY, sizeof(SharedData), 0666);
    if (shmid < 0) {
        perror("Erreur shmget");
        return 1;
    }

    SharedData *shared = (SharedData *) shmat(shmid, NULL, 0);
    if (shared == (void *)-1) {
        perror("Erreur shmat");
        return 1;
    }

    printf("[INFO] Ordonnanceur connecté à la mémoire partagée.\n");

    if (shared->process_count == 0) {
        printf("[INFO] Aucun processus dans la file.\n");
        return 0;
    }

    switch (shared->algo) {
        case 0:
            ordonnance_fifo(shared);
            break;
        case 1:
            ordonnance_rr(shared);
            break;
        case 2:
            ordonnance_priorite(shared);
            break;
        default:
            printf("Algorithme inconnu: %d\n", shared->algo);
    }

    shmdt(shared);
    return 0;
}