#ifndef QUEUE_H
#define QUEUE_H

#include "hal_types.h"

typedef struct StructMessage
{
    uint8_t* buffer;
    uint8_t is_multicast;
    uint8_t rssi;
    struct StructMessage* next;
} Message;

typedef struct StructQueue
{
    unsigned int size;
    Message* head;
    Message* tail;
} Queue;

Queue* queue_add_element(Queue* q, uint8_t* buffer, uint8_t rssi, uint8_t is_multicast);
Message* queue_remove_element(Queue* q);
Queue* queue_new(void);
Queue* queue_free(Queue* q);

#endif