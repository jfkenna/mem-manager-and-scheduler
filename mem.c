//includes
#include "stdio.h"
#include "stdlib.h"
#include "unistd.h" //option processing

//defines
#define MAX_NUM_PROCESSES 1000
#define RESIZE_MULTIPLIER 2
#define PAGE_ARRAY_INIT_SIZE 10

//memory config
#define MEM_UNLIMITED 'u'
#define MEM_SWAPPING 'p'
#define MEM_VIRTUAL 'v'
#define MEM_CUSTOM 'c'

//scheduler config
#define SCHEDULER_FCFS 'f'
#define SCHEDULER_RR 'r'
#define SCHEDULER_CUSTOM 'c'

//debugging
#define DEBUG 0

//***********************************************************************************************
//process information struct
typedef struct process{
    unsigned long time_arrived;
    unsigned long process_id;
    unsigned long memory_size_req;
    unsigned long job_time;
} process;

void print_process(process* target_process){
    printf("%lu %lu %lu %lu", target_process->time_arrived, target_process->process_id, 
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
    unsigned long len;
};

process_queue* construct_queue(){
    process_queue* new_queue = malloc(sizeof(process_queue));
    new_queue->front = NULL;
    new_queue->back = NULL;
    new_queue->len = 0;
    return new_queue;

}

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

process* queue_dequeue(process_queue* queue){
    //if queue has nothing to dequeue
    if (queue->len == 0){
        return NULL;
    }

    process* return_val = queue->front->value;
    if (queue->len > 1){
        queue->front->prev->next = NULL;
        queue->front = queue->front->prev;
    }
    
    if (queue->len == 1){
        queue->back = NULL;
        queue->front = NULL;
    }

    queue->len -= 1;
    return return_val;
}

void print_queue(process_queue* queue){
    printf("\nprinting from the back of the queue");
    queue_node* cur = queue->back;
    unsigned long i = 0;
    while(cur != NULL){
        printf("\n[%lu]: ", i);
        print_process(cur->value);
        cur = cur->next;
        i += 1;
    }
    printf("\nprint complete...");
}


//***********************************************************************************************
//memory storage datatype
typedef struct Sorted_mem_pages sorted_mem_pages;
struct Sorted_mem_pages{
    unsigned long len; //number of elements
    unsigned long alloced_len; //number of elements that can fit in allocated memory
    unsigned long* page_array;
};

sorted_mem_pages* construct_mem_pages(){
    sorted_mem_pages* new_pages = malloc(sizeof(sorted_mem_pages));
    new_pages->len = 0;
    new_pages->alloced_len = 0;
    new_pages->page_array = NULL;
    return new_pages;
}

void page_array_insert(sorted_mem_pages* page_storage, int page_number){
    //insert first element
    if (page_storage->len == 0){
        //set initial size
        if (page_storage->alloced_len == 0){
            page_storage->page_array = malloc(PAGE_ARRAY_INIT_SIZE * sizeof(unsigned long));
            page_storage->alloced_len = PAGE_ARRAY_INIT_SIZE;
        }
        page_storage->page_array[0] = page_number;
        page_storage->len = 1;
    }else{
        //=================================================================================
        //the following binary search for insertion location is a modified form of code obtained from
        //https://stackoverflow.com/questions/24868637/inserting-in-to-an-ordered-array-using-binary-search
        unsigned long mid;
        unsigned long low = 0;
        unsigned long high = page_storage->len-1;
        while (low != high){
            mid = low/2 + high/2;
            if (page_storage->page_array[mid] <= page_number){
                low = mid + 1;
            }else{
                high = mid;
            }
        }
        unsigned long insert_index = mid;
        //=============================end modified code===================================

        //reallocate and insert
        //prob have off by 1 bugs here
        if (page_storage->alloced_len < 1 + page_storage->len){
            unsigned long* new_storage_array = malloc(((page_storage->len + 1) * RESIZE_MULTIPLIER) * sizeof(unsigned long));
            page_storage->alloced_len = (page_storage->len + 1) * RESIZE_MULTIPLIER;

            unsigned long did_insert = 0;
            for (unsigned long i = 0; i < page_storage->len; i++){
                if (i == insert_index){
                    new_storage_array[i] = page_number;
                    did_insert = 1;
                }
                new_storage_array[i+did_insert] = page_storage->page_array[i+did_insert];
            }
            free(page_storage->page_array);
            page_storage->page_array = new_storage_array;
        }
        //if no reallocation is required

        //store values that will be shifted
        unsigned long j;
        unsigned long* tmp = malloc((page_storage->len - insert_index + 1) * sizeof(unsigned long));
        for (j = insert_index; j < page_storage->len; j++ ){
            tmp[j] = page_storage->page_array[j]; 
        }
        //insert
        page_storage->page_array[j] = page_number;

        //rewrite shifted values
        for (unsigned long k = insert_index+1; k < page_storage->len + 1; k++){
            page_storage->page_array[k] = tmp[k];
        }
    }
}


//hash table (closer to a lookup table as the hashing function is x => x)
sorted_mem_pages** create_hash_table(){
    sorted_mem_pages** mem_hash_table = malloc((MAX_NUM_PROCESSES) * sizeof(sorted_mem_pages));
    //populate hash table with empty lists
    for (unsigned long i = 0; i < MAX_NUM_PROCESSES; i++){
        mem_hash_table[i] = construct_mem_pages();
    }
    return mem_hash_table;
}


//ez
void allocate_mem_to_process(sorted_mem_pages** mem_hash_table, unsigned long process_id, unsigned long page_number){
    page_array_insert(mem_hash_table[process_id], page_number);
}

//***********************************************************************************************
//hidden to actual assignment, stores incoming data with having to constantly do IO
process_queue* load_processes(char* filename){
    //open file
    FILE* file = fopen(filename, "r");

    //if file open failed
    if (file == NULL){
        if (DEBUG){
            printf("\nnull file, returning...");
        }
        return NULL;
    }
    
    //load processes
    process_queue* incoming_process_queue = construct_queue();
    for (process* new_process = malloc(sizeof(process)); fscanf(file,"%lu %lu %lu %lu\r\n", &new_process->time_arrived, 
            &new_process->process_id, &new_process->memory_size_req, 
            &new_process->job_time) == 4;new_process = malloc(sizeof(process))){
        //enqueue process
        queue_enqueue(incoming_process_queue, new_process);
        
    }
    return incoming_process_queue;
}

//***********************************************************************************************
//dummy function so code compiles, replace later
unsigned long load_memory(process* cur_process, char memory_manager){
    if (DEBUG){
        printf("loaded memory...");
    }
    
    return 0;
}

//***********************************************************************************************
//first come first served scheduler

//can prob do faster with some kind of lookup or special ordering in data struct, but O(n) is pretty fast anyway so who cares
unsigned long processes_waiting(unsigned long current_time, process_queue* incoming_process_queue){
    if (incoming_process_queue->len == 0){
        return 0;
    }
    unsigned long n_processes_waiting = 0;
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


void first_come_first_served(process_queue* incoming_process_queue, char memory_manager){
    unsigned long current_time = 0;
    unsigned long n_processes_waiting;
    process* cur_process = queue_dequeue(incoming_process_queue);
    while (cur_process != NULL){
        //simulate waiting for new process
        if (cur_process->time_arrived > current_time){
            if (DEBUG){
                printf("\nwaiting for new process...");
            }
            
            current_time += cur_process->time_arrived - current_time;
        }

        //print generic process start info
        printf("%lu, RUNNING, id=<%lu>, remaining-time=<%lu>\n", current_time, cur_process->process_id, cur_process->job_time);
        
        //load required pages
        if (memory_manager != MEM_UNLIMITED){
            current_time += load_memory(cur_process, memory_manager);
        }
        
        //simulate time spent loading pages and running job
        current_time += cur_process->job_time;
        cur_process->job_time = 0;

        //count how many processes are waiting
        n_processes_waiting = processes_waiting(current_time, incoming_process_queue);

        //print process complete info
        printf("%lu, FINISHED, id=<%lu>, proc-remaining=<%lu>\n", current_time, cur_process->process_id, n_processes_waiting);

        //get next enqueued process
        cur_process = queue_dequeue(incoming_process_queue);
    }
}


//***********************************************************************************************
//round robin scheduler

unsigned long max(unsigned long a, unsigned long b){
    if (a >= b){
        return a; 
    }
    return b;
}

void enqueue_arrived_processes(unsigned long current_time, process_queue* working_queue, process_queue* incoming_process_queue){
    if (incoming_process_queue->len == 0){
        if (DEBUG){
            printf("\nincoming queue is empty");
        }
        
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

void round_robin(process_queue* incoming_process_queue, unsigned long quantum, char memory_manager){
    unsigned long current_time = 0;
    process* cur_process;

    //used to store processes that have actually arrived
    process_queue* working_queue = construct_queue();

    //get first process
    enqueue_arrived_processes(current_time, working_queue, incoming_process_queue);
    cur_process = queue_dequeue(working_queue);
    printf("%lu, RUNNING, id=<%lu>, remaining-time=<%lu>\n", current_time, cur_process->process_id, cur_process->job_time);

    //run until no processes remain
    while (1==1){
        //if job will be finished in this quantum
        if (cur_process->job_time <= quantum){
            //update time values and enqueue new processes
            current_time += cur_process->job_time;
            cur_process->job_time = 0;
            enqueue_arrived_processes(current_time, working_queue, incoming_process_queue);
            printf("%lu, FINISHED, id=<%lu>, proc-remaining=<%lu>\n", current_time, cur_process->process_id, working_queue->len);

            //if no jobs can be swapped to, simulate waiting for the next job to arrive
            if (working_queue->len == 0){
                if (DEBUG){
                    printf("\nnothing in working queue...");
                }
                
                //wait for processes that haven't yet arrived
                if (incoming_process_queue->len != 0){
                    if (DEBUG){
                        printf("\nwaiting for processes...");
                    }
                    
                    current_time += (incoming_process_queue->front->value->time_arrived - current_time);
                    //enqueue arrived processes
                    enqueue_arrived_processes(current_time, working_queue, incoming_process_queue);
                }else{
                    //if no jobs exist to work on and no jobs will arrive in the future, return
                    if (DEBUG){
                        printf("\nall processes complete");
                    }                    
                    return;
                }
            }
            //load next process
            cur_process = queue_dequeue(working_queue);
            if (memory_manager != MEM_UNLIMITED){
                current_time += load_memory(cur_process, memory_manager);
            }
            printf("%lu, RUNNING, id=<%lu>, remaining-time=<%lu>\n", current_time, cur_process->process_id, cur_process->job_time);
            

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
                if (memory_manager != MEM_UNLIMITED){
                    current_time += load_memory(cur_process, memory_manager);
                }
                printf("%lu, RUNNING, id=<%lu>, remaining-time=<%lu>\n", current_time, cur_process->process_id, cur_process->job_time);
            }
        }
        
    }
}

//TODO!!! ADD LOGIC TO ENQUEUE TO MAINTAIN PROCESS_ID ORDER WITHIN THE QUEUE

//***********************************************************************************************
int main(int argc, char** argv){
    //disable print buffer
    setvbuf(stdout, NULL, _IONBF, 0);

    //get control args
    int opt;
    unsigned long quantum = 0;
    unsigned long memory_size = 0;
    char memory_allocator = '\0';
    char scheduling_algorithm = '\0';
    char* filename = NULL;
    while ((opt = getopt(argc, argv, "f:a:m:s:q::")) != -1){
        if (DEBUG){
            printf("\nnew param - ");
        }
        
        if (opt == 'f'){
            filename = optarg;
            if (DEBUG){
                printf("filename: %s", filename);
            }
            
        }
        if (opt == 'a'){
            scheduling_algorithm = optarg[0];
            if (DEBUG){
                printf("scheduling algorithm: %c", scheduling_algorithm);
            }
            
        }
        if (opt == 'm'){
            memory_allocator = optarg[0];
            if (DEBUG){
                printf("allocator type: %c", memory_allocator);
            }
            
        }
        if (opt == 's'){
            memory_size = (unsigned long)*optarg;
            if (DEBUG){
                printf("memory size: %lu", memory_size);
            }
            
        }
        if (opt == 'q'){
            quantum = (unsigned long)*optarg;
            if (DEBUG){
                printf("quantum: %lu", quantum);
            }
            
        }
        if (opt == '?'){
            if (DEBUG){
                printf("1 or more args not recognised... returning");
            }
            
            return 1;
        }
    }

    if (scheduling_algorithm == '\0' || memory_allocator == '\0' || filename == NULL || memory_size == 0){
        if (DEBUG){
            printf("\nan arg was unset or memory size was 0... returning");
        }
        
        return 1;
    }

    //read data
    if (DEBUG){
        printf("\nattempting to read file: \"%s\"", filename);
    }
    
    process_queue* incoming_process_queue = load_processes(filename);
    if (DEBUG){
        print_queue(incoming_process_queue);
    }
    

    //handle errors during read
    if (incoming_process_queue->len == 0){
        if (DEBUG){
            printf("error during file read... returning...");
        }
        return 1;
    }

    //run round robin scheduler with unlimited memory
    if (scheduling_algorithm == SCHEDULER_RR){
        round_robin(incoming_process_queue, quantum, memory_allocator);
    }
    if (scheduling_algorithm == SCHEDULER_FCFS){
        first_come_first_served(incoming_process_queue, memory_allocator);
    }
    if (scheduling_algorithm == SCHEDULER_CUSTOM){
        if (DEBUG){
            printf("custom scheduler is not yet implemented yet");
        }
    }
    
    return 0;
}