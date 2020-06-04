#include "queue.h"
#include "stdlib.h"
#include "stdio.h"


//***********************************************************************************************
//queue constructor

process_queue* construct_queue(){
    process_queue* new_queue = malloc(sizeof(process_queue));
    new_queue->front = NULL;
    new_queue->back = NULL;
    new_queue->len = 0;
    return new_queue;

}

//***********************************************************************************************
//enqueues process into queue
void queue_enqueue(process_queue* queue, process* new_process){
    queue_node* new_node = malloc(sizeof(queue_node));
    new_node->value = new_process;
    new_node->prev = NULL;
    if (queue->len != 0){
        new_node->next = queue->back;
        queue->back->prev = new_node;
    }else{
        new_node->next = NULL;
        queue->front = new_node;
    }
    queue->back = new_node; 
    queue->len += 1;
}


//***********************************************************************************************
//pops process from queue
process* queue_dequeue(process_queue* queue){
    if (queue->len == 0){
        return NULL;
    }

    process* return_val = queue->front->value;
    if (queue->len > 1){
        queue->front->prev->next = NULL;
        queue_node* tmp = queue->front->prev;
        free(queue->front);
        queue->front = tmp;
    }
    
    if (queue->len == 1){
        free(queue->front);
        queue->back = NULL;
        queue->front = NULL;
    }

    queue->len -= 1;
    return return_val;
}


//***********************************************************************************************
//removes provided node from queue
void remove_node(process_queue* working_queue, queue_node* target_node){
    //edge case 1 --> 0
    if (working_queue->len == 1){
        working_queue->front = NULL;
        working_queue->back = NULL;
    }

    //edge case 2 --> 1
    queue_node* remaining = NULL;
    if (working_queue->len == 2){
        if (target_node == working_queue->back){
            remaining = target_node->next;
        }else{
            remaining = target_node->prev;
        }
        remaining->prev = NULL;
        remaining->next = NULL;
        working_queue->front = remaining;
        working_queue->back = remaining;
    }

    if (working_queue->len > 2){
        if (target_node == working_queue->back){
            target_node->next->prev = NULL;
            working_queue->back = target_node->next;
        }else{
            if (target_node == working_queue->front){
                target_node->prev->next = NULL;
                working_queue->front = target_node->prev;
            }else{
                target_node->prev->next = target_node->next;
                target_node->next->prev = target_node->prev;
            }
        }
    }
    free(target_node);
    working_queue->len -= 1;
}

//***********************************************************************************************
//returns the process in the queue with the shortest job-time
process* queue_dequeue_shortest_job(process_queue* working_queue){
    if (working_queue->len == 0){
        return NULL;
    }
    queue_node* min_process = working_queue->front;
    queue_node* cur_process = working_queue->front;

    //find minimum 
    while(cur_process != NULL){
        if (cur_process->value->initial_job_time < min_process->value->initial_job_time){
            min_process = cur_process;
        }
        cur_process = cur_process->prev;
    }

    process* export = min_process->value;
    remove_node(working_queue, min_process);
    return export;
}
