#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <string.h>
#include <unistd.h>

#define SHM_KEY 0x1234
#define MAX_PROCESSES 10

// États possibles
typedef enum {READY, RUNNING, WAITING, TERMINATED} ProcessState;

// Structure d’un processus simulé
typedef struct {
    int pid;
    int arrival_time;
    int burst_time;
    int priority;
    ProcessState state;
    int remaining_time;
} Process;

// Structure partagée
typedef struct {
    int algo; // 0: FIFO, 1: RR, 2: PRIORITY
    int process_count;
    Process processes[MAX_PROCESSES];
} SharedData;

int main() {
    int shmid = shmget(SHM_KEY, sizeof(SharedData), IPC_CREAT | 0666);
    if (shmid < 0) {
        perror("shmget failed");
        exit(1);
    }

    SharedData *shared = (SharedData *) shmat(shmid, NULL, 0);
    if (shared == (void *) -1) {
        perror("shmat failed");
        exit(1);
    }

    // Initialisation
    shared->algo = 0; // FIFO
    shared->process_count = 3;

    shared->processes[0] = (Process){.pid = 1, .arrival_time = 0, .burst_time = 5, .priority = 2, .state = READY, .remaining_time = 5};
    shared->processes[1] = (Process){.pid = 2, .arrival_time = 1, .burst_time = 3, .priority = 1, .state = READY, .remaining_time = 3};
    shared->processes[2] = (Process){.pid = 3, .arrival_time = 2, .burst_time = 4, .priority = 3, .state = READY, .remaining_time = 4};

    printf("[INIT] Mémoire partagée initialisée avec 3 processus.\n");

    shmdt(shared); // Détacher
    return 0;
}