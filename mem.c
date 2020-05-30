//includes
#include "stdio.h"
#include "stdlib.h"
#include "unistd.h" //option processing
#include "string.h"


//memory management datatype config
#define MAX_NUM_PROCESSES 1000
#define RESIZE_MULTIPLIER 2
#define PAGE_ARRAY_INIT_SIZE 10
#define LOADING_COST 2



//treat max unsigned long as missing value
#define EMPTY_VALUE 4294967295 //i'm aware that this value of ULONG_MAX isn't guaranteed, fk it tho

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
//utility function

unsigned long max(unsigned long a, unsigned long b){
    if (a >= b){
        return a; 
    }
    return b;
}

//***********************************************************************************************
//process information data structure

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
//queue data structure and helper functions

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
//memory storage data structures and helper functions

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

//maintain reverse sorted order on insertion
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
            if (page_storage->page_array[mid] >= page_number){ //use >= rather than <= to search reverse ordered array 
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
            //expand allocation, fill with empty values and write over
            unsigned long* new_storage_array = malloc(((page_storage->len + 1) * RESIZE_MULTIPLIER) * sizeof(unsigned long));
            page_storage->alloced_len = (page_storage->len + 1) * RESIZE_MULTIPLIER;
            for (unsigned long writeover; writeover < page_storage->len+1; writeover++){
                page_storage->page_array[writeover] = EMPTY_VALUE;
            }

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


//always remove last element
unsigned long page_array_pop_last(sorted_mem_pages* page_storage){
    unsigned long popped_page = page_storage->page_array[page_storage->len - 1];
    page_storage->page_array[page_storage->len - 1] = EMPTY_VALUE;
    page_storage->len -= 1;
    return popped_page;
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


//fill initial memory pool with pages (maintain reverse sorted ordering)
sorted_mem_pages* populate_free_memory_pool(unsigned long available_memory){
    sorted_mem_pages* free_memory_pool = construct_mem_pages();
    unsigned long* page_array = malloc((available_memory/4) * sizeof(unsigned long));

    //ensure initial fill is sorted in reverse order
    for (unsigned long i = (available_memory/4) - 1; i >= 0; i--){
        page_array[i] = i;
    }
    //ready to output
    free_memory_pool->len = available_memory/4;
    free_memory_pool->alloced_len = available_memory/4;
    free_memory_pool->page_array = page_array;
    return free_memory_pool;
}


//***********************************************************************************************
//swapping

unsigned long mem_swap(sorted_mem_pages** mem_hash_table, sorted_mem_pages* free_memory_pool, process_queue* working_queue, unsigned long requesting_process_id, unsigned long pages_required, char memory_manager){
    //remove least recently executed ? i feel it should be most recently executed but i fill follow the spec!!!
    queue_node* removal_target_process = working_queue->front;
    
    unsigned long page_to_swap;
    unsigned long pages_swapped = 0;
    //until enough memory is freed, free all memory allocated single processes
    while (free_memory_pool->len < pages_required){
        //remove all pages allocated to this process and attach them to new process
        while (mem_hash_table[removal_target_process->value->process_id]->len > 0){
            page_to_swap = page_array_pop_last(mem_hash_table[removal_target_process->value->process_id]);
            page_array_insert(mem_hash_table[requesting_process_id], page_to_swap);
            pages_swapped += 1;
            printf("\nswapped page from %lu to %lu", removal_target_process->value->process_id, requesting_process_id);
        }
        removal_target_process = removal_target_process->prev;
    }
    return pages_swapped;
}


//***********************************************************************************************
//selects between different memory loading types, returns cost
unsigned long load_memory(process* requesting_process, sorted_mem_pages** mem_hash_table, sorted_mem_pages* free_memory_pool, process_queue* working_queue, char memory_manager){
    unsigned long free_page;
    unsigned long cost;
    unsigned long initial_pages_required = (requesting_process->memory_size_req / 4) - mem_hash_table[requesting_process->process_id]->len;
    unsigned long current_pages_required = initial_pages_required;

    //don't need to manage memory when set to unlimited
    if (memory_manager == MEM_UNLIMITED){
        return 0;
    }

    //load pages from free memory until it's exhausted or the program has enough memory requirements
    //don't add cost as free memory doesn't need any disk stuff
    while (free_memory_pool->len > 0 && current_pages_required > 0){
        free_page = page_array_pop_last(free_memory_pool);
        page_array_insert(mem_hash_table[requesting_process->process_id], free_page);
        current_pages_required -= 1;
    }

    //if requirements could be fulfilled from free memory, return early
    if (current_pages_required == 0){
        cost = initial_pages_required * LOADING_COST;
        return cost;
    }

    //select between memory management methods
    if (memory_manager == MEM_SWAPPING){
        if (DEBUG){
            printf("\nswapping memory...");
        }
        mem_swap(mem_hash_table, free_memory_pool, working_queue, requesting_process->process_id, current_pages_required, memory_manager);
        //only the pages required by the process add to the cost, even if we swap more
        cost = initial_pages_required * LOADING_COST;
    }
    if (memory_manager == MEM_VIRTUAL){
        printf("\nnot implemented yet...");
    }

    return cost;
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
        printf("%lu, RUNNING, id=%lu, remaining-time=%lu\n", current_time, cur_process->process_id, cur_process->job_time);
        
        //load required pages
        if (memory_manager != MEM_UNLIMITED){
            //NEED TO REWRITE SO WORKING QUEUE IS MEANINGFUL HERE
            //need to do other integration stuff also
            //current_time += load_memory(cur_process, mem_hash_table, free_memory_pool, working_queue, memory_manager);
        }
        
        //simulate time spent loading pages and running job
        current_time += cur_process->job_time;
        cur_process->job_time = 0;

        //count how many processes are waiting
        n_processes_waiting = processes_waiting(current_time, incoming_process_queue);

        //print process complete info
        printf("%lu, FINISHED, id=%lu, proc-remaining=%lu\n", current_time, cur_process->process_id, n_processes_waiting);

        //get next enqueued process
        cur_process = queue_dequeue(incoming_process_queue);
    }
}


//***********************************************************************************************
//round robin scheduler

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
    printf("%lu, RUNNING, id=%lu, remaining-time=%lu\n", current_time, cur_process->process_id, cur_process->job_time);

    //run until no processes remain
    while (1==1){
        //if job will be finished in this quantum
        if (cur_process->job_time <= quantum){
            //update time values and enqueue new processes
            current_time += cur_process->job_time;
            cur_process->job_time = 0;
            enqueue_arrived_processes(current_time, working_queue, incoming_process_queue);
            printf("%lu, FINISHED, id=%lu, proc-remaining=%lu\n", current_time, cur_process->process_id, working_queue->len);

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
                //need to do integration work on this
                //current_time += load_memory(cur_process, mem_hash_table, free_memory_pool, working_queue, memory_manager);
            }
            printf("%lu, RUNNING, id=%lu, remaining-time=%lu\n", current_time, cur_process->process_id, cur_process->job_time);
            

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
                    //need to do integration work on this
                    //current_time += load_memory(cur_process, mem_hash_table, free_memory_pool, working_queue, memory_manager);
                }
                printf("%lu, RUNNING, id=%lu, remaining-time=%lu\n", current_time, cur_process->process_id, cur_process->job_time);
            }
        }
        
    }
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
int main(int argc, char** argv){
    //disable print buffer
    setvbuf(stdout, NULL, _IONBF, 0);
    if (DEBUG){
        printf("%lu", EMPTY_VALUE);
    }
    
    //get control args
    int opt;
    unsigned long quantum = 0;
    unsigned long memory_size = 0;
    char memory_manager = '\0';
    char scheduling_algorithm = '\0';
    char* filename = NULL;
    while ((opt = getopt(argc, argv, "f:a:m:s:q:")) != -1){
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
            memory_manager = optarg[0];
            if (DEBUG){
                printf("allocator type: %c", memory_manager);
            }
            
        }
        if (opt == 's'){
            memory_size = strtoul(optarg, NULL, 0);
            if (DEBUG){
                printf("memory size: %lu", memory_size);
            }
            
        }
        if (opt == 'q'){
            quantum = strtoul(optarg, NULL, 0);
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

    if (scheduling_algorithm == '\0' || memory_manager == '\0' || filename == NULL){
        if (DEBUG){
            printf("\na required arg wasn't set... returning");
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
        round_robin(incoming_process_queue, quantum, memory_manager);
    }
    if (scheduling_algorithm == SCHEDULER_FCFS){
        first_come_first_served(incoming_process_queue, memory_manager);
    }
    if (scheduling_algorithm == SCHEDULER_CUSTOM){
        if (DEBUG){
            printf("custom scheduler is not yet implemented yet");
        }
    }
    
    return 0;
}