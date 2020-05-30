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

void print_process(process* target_process){
    printf("%d %d %d %d", target_process->time_arrived, target_process->process_id, 
    target_process->memory_size_req, target_process->job_time);
}


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

process_queue* construct_queue(){
    process_queue* new_queue = malloc(sizeof(process_queue));
    new_queue->front = NULL;
    new_queue->back = NULL;
    new_queue->len = 0;
    return new_queue;

}

void queue_enqueue(process_queue* queue, process* new_process){
    printf("\nenqueuing/");
    print_process(new_process);
    //create new node
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

process* queue_dequeue(process_queue* queue){
    //if queue has nothing to dequeue
    if (queue->len == 0){
        return NULL;
    }

    process* return_val = queue->front->value;
    if (queue->len > 1){
        queue->front->prev->next = NULL;
        queue->front = queue->front->prev;
        printf("\ndequeue complete - new front is: ");
        print_process(queue->front->value);
    }
    
    if (queue->len == 1){
        queue->back = NULL;
        queue->front = NULL;
        printf("\ndequeue complete - queue is now empty");
    }

    queue->len -= 1;
    return return_val;
}

void print_queue(process_queue* queue){
    printf("\nprinting from the back of the queue");
    queue_node* cur = queue->back;
    int i = 0;
    while(cur != NULL){
        printf("\n[%d]: ", i);
        print_process(cur->value);
        cur = cur->next;
        i += 1;
    }
    printf("\nprint complete...");
}


//***********************************************************************************************
//hidden to actual assignment, stores incoming data with having to constantly do IO
process_queue* load_processes(char* filename){
    //open file
    FILE* file = fopen(filename, "r");

    //if file open failed
    if (file == NULL){
        printf("\nnull file, returning...");
        return NULL;
    }
    
    //load processes
    process_queue* incoming_process_queue = construct_queue();
    for (process* new_process = malloc(sizeof(process)); fscanf(file,"%d %d %d %d\r\n", &new_process->time_arrived, 
            &new_process->process_id, &new_process->memory_size_req, 
            &new_process->job_time) == 4;new_process = malloc(sizeof(process))){
        //enqueue process
        queue_enqueue(incoming_process_queue, new_process);
        
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

//can prob do faster with some kind of lookup or special ordering in data struct, but O(n) is pretty fast anyway so who cares
int processes_waiting(int current_time, process_queue* incoming_process_queue){
    if (incoming_process_queue->len == 0){
        return 0;
    }
    int n_processes_waiting = 0;
    queue_node* cur = incoming_process_queue->front;
    while (cur != NULL){
        if (cur->value->time_arrived <= current_time){
            n_processes_waiting += 1;
            cur = cur->prev;
        }else{
            return n_processes_waiting;
        }
    }
    return n_processes_waiting;
}


void first_come_first_served(process_queue* incoming_process_queue, int memory_manager_type){
    int current_time = 0;
    int n_processes_waiting;
    process* cur_process = queue_dequeue(incoming_process_queue);
    while (cur_process != NULL){
        //simulate waiting for new process
        if (cur_process->time_arrived > current_time){
            printf("\nwaiting for new process...");
            current_time += cur_process->time_arrived - current_time;
        }

        //print generic process start info
        printf("\ncurrent_time is %d , program running, id %d, remaining time %d", current_time, cur_process->process_id, cur_process->job_time);
        
        //load required pages
        if (memory_manager_type != MEM_UNLIMITED){
            current_time += load_memory(cur_process, memory_manager_type);
        }
        
        //simulate time spent loading pages and running job
        current_time += cur_process->job_time;
        cur_process->job_time = 0;

        //count how many processes are waiting
        n_processes_waiting = processes_waiting(current_time, incoming_process_queue);

        //print process complete info
        printf("\ncurrent time is %d, process complete, id %d, processes remaining %d", current_time, cur_process->process_id, n_processes_waiting);

        //get next enqueued process
        cur_process = queue_dequeue(incoming_process_queue);
    }
}


int max(int a, int b){
    if (a >= b){
        return a; 
    }
    return b;
}


void enqueue_arrived_processes(int current_time, process_queue* working_queue, process_queue* incoming_process_queue){
    if (incoming_process_queue->len == 0){
        printf("\nincoming queue is empty");
        return;
    }

    queue_node* cur = incoming_process_queue->front;
    while (cur != NULL){
        //swap from incoming processes to working processes if process would have arrived by now
        if (cur->value->time_arrived <= current_time){
            queue_enqueue(working_queue, queue_dequeue(incoming_process_queue));
        }else{
            return;
        }
        cur = incoming_process_queue->front;
    }
}

void round_robin(process_queue* incoming_process_queue, int quantum, int memory_manager_type){
    int current_time = 0;
    process* cur_process;

    //used to store processes that have actually arrived
    process_queue* working_queue = construct_queue();

    //get first process
    cur_process = queue_dequeue(working_queue);

    //run until no processes remain
    while (1==1){
        //if job will be finished in this quantum
        if (cur_process->job_time <= quantum){
            //update time values and enqueue new processes
            current_time += cur_process->job_time;
            cur_process->job_time = 0;
            enqueue_arrived_processes(current_time, working_queue, incoming_process_queue);
            printf("\ncurrent time is %d, process complete, id %d, processes remaining %d", current_time, cur_process->process_id, incoming_process_queue->len);

            //if no jobs can be swapped to, simulate waiting for the next job to arrive
            if (working_queue->len == 0){
                printf("\nnothing in working queue...");
                //wait for processes that haven't yet arrived
                if (incoming_process_queue->len != 0){
                    printf("\nwaiting for processes...");
                    current_time += (incoming_process_queue->front->value->time_arrived - current_time);
                    //load arrived processes and continue
                    enqueue_arrived_processes(current_time, working_queue, incoming_process_queue);
                    cur_process = queue_dequeue(working_queue);
                    printf("\ncurrent time is %d, process running, id %d, remaining time %d", current_time, cur_process->process_id, cur_process->job_time);
                }else{
                    //if no jobs exist to work on and no jobs will arrive in the future, return
                    printf("\nall processes complete");
                    return;
                }
            }
            //enqueue next job
            cur_process = queue_dequeue(working_queue);
            current_time += load_memory(cur_process, memory_manager_type);

        //if job won't be finished this quantum, shuffle it to the back and select a new job
        }else{
            //update time values and enqueue new processes
            current_time += quantum;
            cur_process->job_time -= quantum;
            enqueue_arrived_processes(current_time, working_queue, incoming_process_queue);

            //only change process and report change if a new process can be swapped to (in the case that working_queue is 0 after removal of cur_process and addition of new processes, no processes can be swapped to)
            if (working_queue->len != 0){
                //place process at back of queue and announce return from suspension
                queue_enqueue(working_queue, cur_process);
                cur_process = queue_dequeue(working_queue);
                printf("\ncurrent time is %d, process running, id %d, remaining time %d", current_time, cur_process->process_id, cur_process->job_time);
            }
        }
        
    }

}

//TODO!!! ADD LOGIC TO ENQUEUE TO MAINTAIN PROCESS_ID ORDER WITHIN THE QUEUE

//***********************************************************************************************
int main(int argc, char** argv){
    //disable print buffer
    setvbuf(stdout, NULL, _IONBF, 0);

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
    //first_come_first_served(incoming_process_queue, MEM_UNLIMITED);

    //run round robin scheduler with unlimited memory
    round_robin(incoming_process_queue, 1, MEM_UNLIMITED);
    return 0;
}