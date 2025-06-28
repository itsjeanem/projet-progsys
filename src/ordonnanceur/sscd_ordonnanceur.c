#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <string.h>
#include <time.h>

#define MAX_PROCESSES 10
#define TIME_QUANTUM 2
#define SHM_KEY 0x1234

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

const char* get_state_string(ProcessState state) {
    switch (state) {
        case READY: return "READY";
        case RUNNING: return "RUNNING";
        case WAITING: return "WAITING";
        case TERMINATED: return "TERMINATED";
        default: return "UNKNOWN";
    }
}

void afficher_stats(int wt_total, int tt_total, int n, double start_time, double end_time) {
    if (n == 0) return;
    double avg_wt = (double)wt_total / n;
    double avg_tt = (double)tt_total / n;
    double total_time = end_time - start_time;
    double throughput = (total_time > 0) ? (double)n / total_time : 0;

    printf("\n--- Résultats ---\n");
    printf("Temps moyen d'attente    : %.2f\n", avg_wt);
    printf("Temps moyen de rotation  : %.2f\n", avg_tt);
    printf("Débit (throughput)       : %.2f processus/sec\n", throughput);
    printf("--------------------------\n\n");
}

// FIFO (inchangé mais optimisé)
void ordonnance_fifo(SharedData *shared) {
    printf("\n--- Ordonnancement: FIFO ---\n");
    int current_time = 0;
    int wt_total = 0, tt_total = 0;
    int completed = 0;
    double start = (double)time(NULL);

    // Tri par temps d'arrivée
    for (int i = 0; i < shared->process_count - 1; i++) {
        for (int j = i + 1; j < shared->process_count; j++) {
            if (shared->processes[i].arrival_time > shared->processes[j].arrival_time) {
                Process temp = shared->processes[i];
                shared->processes[i] = shared->processes[j];
                shared->processes[j] = temp;
            }
        }
    }

    for (int i = 0; i < shared->process_count; i++) {
        Process *p = &shared->processes[i];
        
        if (current_time < p->arrival_time)
            current_time = p->arrival_time;

        p->state = RUNNING;
        printf("[PID=%d] État=%s | Exécution pendant %d unités\n", 
               p->pid, get_state_string(p->state), p->burst_time);
        
        // Simulation sans sleep réel pour éviter les délais
        current_time += p->burst_time;
        
        int turnaround = current_time - p->arrival_time;
        int wait = turnaround - p->burst_time;
        wt_total += wait;
        tt_total += turnaround;
        p->state = TERMINATED;
        completed++;
        
        printf("[PID=%d] Terminé | Attente=%d, Rotation=%d\n", p->pid, wait, turnaround);
    }

    double end = (double)time(NULL);
    afficher_stats(wt_total, tt_total, completed, start, end);
}

// Round Robin corrigé
void ordonnance_rr(SharedData *shared) {
    printf("\n--- Ordonnancement: Round Robin (Quantum=%d) ---\n", TIME_QUANTUM);
    int current_time = 0;
    int wt_total = 0, tt_total = 0;
    int completed = 0;
    int n = shared->process_count;
    double start = (double)time(NULL);

    // Initialiser remaining_time
    for (int i = 0; i < n; i++) {
        shared->processes[i].remaining_time = shared->processes[i].burst_time;
        shared->processes[i].state = READY;
    }

    printf("État initial des processus:\n");
    for (int i = 0; i < n; i++) {
        Process *p = &shared->processes[i];
        printf("[PID=%d] Arrivée=%d, Burst=%d, Restant=%d\n", 
               p->pid, p->arrival_time, p->burst_time, p->remaining_time);
    }

    while (completed < n) {
        int progress_made = 0;

        for (int i = 0; i < n; i++) {
            Process *p = &shared->processes[i];
            
            if (p->state == TERMINATED || p->arrival_time > current_time)
                continue;

            if (p->remaining_time > 0) {
                progress_made = 1;
                int exec_time = (p->remaining_time > TIME_QUANTUM) ? TIME_QUANTUM : p->remaining_time;
                
                p->state = RUNNING;
                printf("[Temps=%d] [PID=%d] RUNNING pendant %d unités (Restant: %d)\n", 
                       current_time, p->pid, exec_time, p->remaining_time);
                
                current_time += exec_time;
                p->remaining_time -= exec_time;

                if (p->remaining_time == 0) {
                    p->state = TERMINATED;
                    int turnaround = current_time - p->arrival_time;
                    int wait = turnaround - p->burst_time;
                    wt_total += wait;
                    tt_total += turnaround;
                    completed++;
                    printf("[PID=%d] TERMINÉ | Attente=%d, Rotation=%d\n", p->pid, wait, turnaround);
                } else {
                    p->state = READY;
                }
            }
        }

        if (!progress_made) {
            current_time++;
        }
    }

    double end = (double)time(NULL);
    afficher_stats(wt_total, tt_total, completed, start, end);
}

// Priorité corrigée
void ordonnance_priorite(SharedData *shared) {
    printf("\n--- Ordonnancement: Priorité (plus petit = plus prioritaire) ---\n");
    int current_time = 0;
    int wt_total = 0, tt_total = 0;
    int completed = 0;
    int n = shared->process_count;
    double start = (double)time(NULL);

    int done[MAX_PROCESSES] = {0};

    printf("État initial des processus:\n");
    for (int i = 0; i < n; i++) {
        Process *p = &shared->processes[i];
        printf("[PID=%d] Arrivée=%d, Burst=%d, Priorité=%d\n", 
               p->pid, p->arrival_time, p->burst_time, p->priority);
    }

    while (completed < n) {
        int idx = -1;
        int highest_priority = 999;

        // Trouver le processus avec la plus haute priorité (valeur la plus petite)
        for (int i = 0; i < n; i++) {
            Process *p = &shared->processes[i];
            if (!done[i] && p->arrival_time <= current_time && p->priority < highest_priority) {
                idx = i;
                highest_priority = p->priority;
            }
        }

        if (idx == -1) {
            // Aucun processus disponible, avancer le temps
            current_time++;
            continue;
        }

        Process *p = &shared->processes[idx];
        p->state = RUNNING;
        printf("[Temps=%d] [PID=%d] PRIORITY=%d | RUNNING pendant %d unités\n", 
               current_time, p->pid, p->priority, p->burst_time);

        current_time += p->burst_time;
        int turnaround = current_time - p->arrival_time;
        int wait = turnaround - p->burst_time;
        wt_total += wait;
        tt_total += turnaround;
        p->state = TERMINATED;
        done[idx] = 1;
        completed++;
        
        printf("[PID=%d] TERMINÉ | Attente=%d, Rotation=%d\n", p->pid, wait, turnaround);
    }

    double end = (double)time(NULL);
    afficher_stats(wt_total, tt_total, completed, start, end);
}

int main() {
    int shmid = shmget(SHM_KEY, sizeof(SharedData), 0666);
    if (shmid < 0) {
        perror("Erreur shmget - la mémoire partagée n'existe pas");
        printf("Assurez-vous que le processus créateur a été lancé en premier.\n");
        return 1;
    }

    SharedData *shared = (SharedData *) shmat(shmid, NULL, 0);
    if (shared == (void *)-1) {
        perror("Erreur shmat");
        return 1;
    }

    printf("[INFO] Ordonnanceur connecté à la mémoire partagée.\n");
    printf("[INFO] Algorithme sélectionné: %d\n", shared->algo);
    printf("[INFO] Nombre de processus: %d\n", shared->process_count);

    if (shared->process_count == 0) {
        printf("[INFO] Aucun processus dans la file.\n");
        shmdt(shared);
        return 0;
    }

    // Afficher les processus disponibles
    printf("\nProcessus disponibles:\n");
    for (int i = 0; i < shared->process_count; i++) {
        Process *p = &shared->processes[i];
        printf("  PID=%d, Arrivée=%d, Burst=%d, Priorité=%d\n", 
               p->pid, p->arrival_time, p->burst_time, p->priority);
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
            printf("Algorithmes disponibles: 0=FIFO, 1=Round Robin, 2=Priorité\n");
    }

    shmdt(shared);
    return 0;
}