#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>

#define PAGE_POOL_SIZE 10
#define MAX_PAGES 100
#define MESSAGE_QUEUE_KEY 1234

// Message pour communication IPC
struct message {
    long mtype;
    char mtext[100];
};

// Structure de page
typedef struct {
    int page_id;
    time_t last_access_time; // pour LRU
    int loaded;
} Page;

Page memory_pool[PAGE_POOL_SIZE];
int page_faults = 0;
int page_hits = 0;

// Initialisation du pool de pages
void init_memory_pool() {
    for (int i = 0; i < PAGE_POOL_SIZE; i++) {
        memory_pool[i].page_id = -1;
        memory_pool[i].loaded = 0;
    }
}

// Vérifie si une page est en mémoire
int is_page_loaded(int page_id) {
    for (int i = 0; i < PAGE_POOL_SIZE; i++) {
        if (memory_pool[i].loaded && memory_pool[i].page_id == page_id) {
            memory_pool[i].last_access_time = time(NULL);
            return 1;
        }
    }
    return 0;
}

// Remplacement FIFO
void replace_page_fifo(int page_id) {
    static int fifo_index = 0;
    memory_pool[fifo_index].page_id = page_id;
    memory_pool[fifo_index].last_access_time = time(NULL);
    memory_pool[fifo_index].loaded = 1;
    fifo_index = (fifo_index + 1) % PAGE_POOL_SIZE;
}

// Remplacement LRU
void replace_page_lru(int page_id) {
    int lru_index = 0;
    time_t oldest_time = memory_pool[0].last_access_time;

    for (int i = 1; i < PAGE_POOL_SIZE; i++) {
        if (memory_pool[i].last_access_time < oldest_time) {
            oldest_time = memory_pool[i].last_access_time;
            lru_index = i;
        }
    }

    memory_pool[lru_index].page_id = page_id;
    memory_pool[lru_index].last_access_time = time(NULL);
    memory_pool[lru_index].loaded = 1;
}

// Traitement d’une requête de page
void handle_page_request(int page_id, const char* algo) {
    if (is_page_loaded(page_id)) {
        page_hits++;
        printf("Page %d hit\n", page_id);
    } else {
        page_faults++;
        printf("Page %d fault\n", page_id);

        if (strcmp(algo, "FIFO") == 0)
            replace_page_fifo(page_id);
        else
            replace_page_lru(page_id);
    }
}

void print_statistics() {
    int total_requests = page_faults + page_hits;
    double hit_ratio = total_requests ? (double)page_hits / total_requests : 0;

    printf("--- Statistiques ---\n");
    printf("Total requests: %d\n", total_requests);
    printf("Page faults: %d\n", page_faults);
    printf("Page hits: %d\n", page_hits);
    printf("Hit ratio: %.2f%%\n", hit_ratio * 100);
}

int main() {
    init_memory_pool();
    int msgid = msgget(MESSAGE_QUEUE_KEY, 0666 | IPC_CREAT);
    struct message msg;

    printf("Gestionnaire de mémoire démarré.\n");

    while (1) {
        if (msgrcv(msgid, &msg, sizeof(msg.mtext), 1, 0) != -1) {
            int page_id;
            char algo[10];
            sscanf(msg.mtext, "%d %s", &page_id, algo);

            handle_page_request(page_id, algo);
            print_statistics();
        } else {
            perror("Erreur de réception de message");
        }
    }

    return 0;
}
