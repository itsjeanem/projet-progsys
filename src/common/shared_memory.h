#ifndef SHARED_MEMORY_H
#define SHARED_MEMORY_H

#include <stdint.h>

#define MAX_TASKS 100

typedef enum
{
    READY,
    RUNNING,
    WAITING,
    COMPLETED,
    FAILED
} task_state_t;

typedef struct
{
    uint32_t id;
    char name[32];
    uint32_t priority;
    uint32_t burst_time;
    uint32_t remaining_time;
    task_state_t state;
} task_t;

typedef struct
{
    task_t tasks[MAX_TASKS];
    uint32_t task_count;
} shared_memory_t;

#endif
