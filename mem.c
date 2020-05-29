//includes
#include "stdio.h"
#include "stdlib.h"

//defines
#define MAX_NUM_PROCESSES 1000

//memory config
#define MEM_UNLIMITED 1
#define MEM_SWAPPING 2
#define MEM_VIRTUAL 3
#define MEM_CUSTOM 4

//scheduler config
#define SCHEDULER_FCFS 1
#define SCHEDULER_RR 2
#define SCHEDULER_CUSTOM 3

//***********************************************************************************************
//process information struct
typedef struct process{
    int time_arrived;
    int process_id;
    int memory_size_req;
    int job_time;
} process;

//***********************************************************************************************
//queue and helper functions

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
    int len;
};

void queue_enqueue(process_queue* queue, process* new_process){
    //create new node
    queue_node* new_node = malloc(sizeof(queue_node));
    new_node->value = new_process;
    new_node->prev = NULL;
    if (queue->len != 0){
        new_node->next = queue->back;
    }else{
        new_node->next = NULL;
    }

    //update queue to reflect new addition
    queue->back->prev = new_node;
    queue->back = new_node; 
}

process* queue_dequeue(process_queue* queue){
    process* return_val = queue->front->value;
    queue->front->prev->next = NULL;
    queue->front = queue->front->prev;
    if (queue->len == 1){
        queue->back = NULL;
    }
    queue->len -= 1;
    return return_val;
}

//***********************************************************************************************
//printing functions
void print_process(process* target_process){
    printf("%d %d %d %d\n", target_process->time_arrived, target_process->process_id, 
    target_process->memory_size_req, target_process->job_time);
}

void print_queue(process_queue* queue){
    printf("printing from the back of the queue");
    queue_node* cur = queue->back;
    while(cur != NULL){
        print_process(cur->value);
        cur = cur->next;
    }
}



//***********************************************************************************************
//hidden to actual assignment, stores incoming data with having to constantly do IO
process_queue* load_processes(char* filename){
    //open file
    FILE* file = fopen(filename, "r");

    //if file open failed
    if (file == NULL){
        printf("null file, returning...");
        return NULL;
    }
    
    //load processes
    process_queue* incoming_process_queue = malloc(sizeof(process_queue));
    for (process* new_process = malloc(sizeof(process)); fscanf(file,"%d %d %d %d\r\n", &new_process->time_arrived, 
            &new_process->process_id, &new_process->memory_size_req, 
            &new_process->job_time) == 4; queue_enqueue(incoming_process_queue, new_process)){
        //create a new process to write to and enqueue
        new_process = malloc(sizeof(process));
    }
    return incoming_process_queue;
}

//***********************************************************************************************
//dummy function so code compiles, replace later
int load_memory(process* cur_process, int memory_manager_type){
    printf("loaded memory...");
    return 0;
}

//***********************************************************************************************
//first come first served scheduler
void first_come_first_served(process_queue* incoming_process_queue, int memory_manager_type){
    int global_time = 0;

    process* cur_process = queue_dequeue(incoming_process_queue);
    while (cur_process != NULL){
        //simulate waiting for new process
        if (cur_process->time_arrived > global_time){
            global_time += cur_process->time_arrived - global_time;
        }

        //print generic process start info
        printf("current_time is %d , program running, id %d, remaining time %d", global_time, cur_process->process_id, cur_process->job_time);
        
        //load required pages
        int load_memory_cost = 0;
        if (memory_manager_type != MEM_UNLIMITED){
            load_memory_cost = load_memory(cur_process, memory_manager_type);
        }
        
        //simulate time spent loading pages and running job
        global_time += load_memory_cost + cur_process->job_time;

        //get next enqueued process
        cur_process = queue_dequeue(incoming_process_queue);
    }
}


//***********************************************************************************************
int main(int argc, char** argv){
    //check correct args are passed
    if (argc != 6){
        printf("incorrect number of arguments, had %d, expected 5\n returning...", argc-1);
        return 1;
    }

    //read data
    printf("attempting to read file: \"%s\"\n", argv[1]);
    process_queue* incoming_process_queue = load_processes(argv[1]);
    print_queue(incoming_process_queue);

    //handle error
    if (incoming_process_queue->len == 0){
        printf("error during file read... returning...");
        return 1;
    }

    //run basic scheduler with unlimited memory
    first_come_first_served(incoming_process_queue, MEM_UNLIMITED);
    return 0;
}