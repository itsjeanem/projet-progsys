#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>

// Format des messages : [TYPE:4][LENGTH:4][TIMESTAMP:8][DATA:LENGTH]

typedef struct
{
    uint32_t type;      // Type de message
    uint32_t length;    // Taille des donnees
    uint64_t timestamp; // Horodatage
    char data[];        // Donnees du message (données variables)
} message_t;

// Types de messages
#define MSG_PING 0x01   // Heartbeat
#define MSG_PONG 0x02   // Reponse heartbeat
#define MSG_CMD 0x03    // Commande
#define MSG_DATA 0x04   // Donnees
#define MSG_ALERT 0x05  // Alerte
#define MSG_STATUS 0x06 // Statut

#endif
