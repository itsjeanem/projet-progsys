#ifndef MEMORY_MESSAGE_H
#define MEMORY_MESSAGE_H

#include <stdint.h>

#define MSG_MEMORY_PRESSURE 0x01

typedef struct
{
    uint32_t type;
    uint32_t page_fault_rate;
    char message[64];
} memory_message_t;

#endif
