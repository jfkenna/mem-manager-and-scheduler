//***********************************************************************************************
//queue data structure and helper functions
#ifndef QUEUE_H
#define QUEUE_H
#include "process.h"

typedef struct Queue_node queue_node;
struct Queue_node{
    queue_node* next;
    queue_node* prev;
    process* value;
};

typedef struct Process_queue process_queue; 
struct Process_queue{
    queue_node* front;
    queue_node* back;
    unsigned long len;
};

process_queue* construct_queue();
void queue_enqueue(process_queue* queue, process* new_process);
process* queue_dequeue(process_queue* queue);
process* queue_dequeue_shortest_job(process_queue* working_queue);
void print_queue(process_queue* queue);
#endif