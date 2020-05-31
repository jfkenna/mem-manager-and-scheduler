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
#define EVICT_ALL 0


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
//utility functions

unsigned long max(unsigned long a, unsigned long b){
    if (a >= b){
        return a; 
    }
    return b;
}

unsigned long min(unsigned long a, unsigned long b){
    if (a <= b){
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
    printf("%lu %lu %lu %lu\n", target_process->time_arrived, target_process->process_id, 
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
    printf("printing queue: back --> front ordering\n");
    queue_node* cur = queue->back;
    unsigned long i = 0;
    while(cur != NULL){
        printf("[%lu]: ", i);
        print_process(cur->value);
        cur = cur->next;
        i += 1;
    }
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
void page_array_insert(sorted_mem_pages* page_storage, unsigned long page_number){
    if (DEBUG){
        //printf("\nwant to insert %lu, array has len %lu and alloc len %lu", page_number, page_storage->len, page_storage->alloced_len);
    }
    //insert first element
    if (page_storage->len == 0){
        if (DEBUG){
            //printf("\ninserting as first element");
        }
        
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
        unsigned long mid = 9999999; //default just for checking
        unsigned long low = 0;
        unsigned long high = page_storage->len; //include the spot just after everything!
        while (low != high){
            mid = low/2 + high/2;
            if (page_storage->page_array[mid] >= page_number){ //use >= rather than <= to search reverse ordered array 
                low = mid + 1;
            }else{
                high = mid;
            }
        }
        unsigned long insert_index = low;
        //=============================end modified code===================================

        if (DEBUG){
            //printf("\ninsert index %lu", insert_index);
        }
        //if not enough memory has been allocated, reallocate and insert
        if (page_storage->alloced_len < 1 + page_storage->len){
            if (DEBUG){
                printf("\n=====================REALLOC=====================");
            }
            
            //expand allocation, fill with empty values
            unsigned long* new_storage_array = malloc((page_storage->len * RESIZE_MULTIPLIER) * sizeof(unsigned long));
            page_storage->alloced_len = page_storage->len * RESIZE_MULTIPLIER;
            for (unsigned long writeover = 0; writeover < page_storage->alloced_len; writeover++){
                new_storage_array[writeover] = EMPTY_VALUE;
            }

            unsigned long insert_offset = 0;
            for (unsigned long i = 0; i < page_storage->len; i++){
                if (i == insert_index){
                    insert_offset = 1;
                }
                
                new_storage_array[i+insert_offset] = page_storage->page_array[i]; 
            }
            new_storage_array[insert_index] = page_number;
            if (DEBUG){
                printf("\n======================page array after realloc and insertion of page #%lu: ", page_number);
            }
            
            //free(page_storage->page_array);
            page_storage->page_array = new_storage_array;
        }else{
            //if no reallocation is required

            //shift over values to make room for new addition
            unsigned long j = page_storage->len-1;
            while (j >= insert_index){
                page_storage->page_array[j+1] = page_storage->page_array[j];
                //avoid overflow from unsigned variable j falling to -1
                if (j == 0){
                    break;
                }else{
                    j -= 1;
                }
            }
            //insert
            page_storage->page_array[insert_index] = page_number;

            if (DEBUG){
                printf("\n==========page array after insertion of page #%lu: ", page_number);
            }
        }
        page_storage->len += 1;
        if (DEBUG){
            for (unsigned long cc = 0; cc < page_storage->len; cc++){
                printf("%lu->", page_storage->page_array[cc]);
            }
            printf("END============\n");
        }
    }
}


//always remove last element
unsigned long page_array_pop_last(sorted_mem_pages* page_storage){
    unsigned long popped_page = page_storage->page_array[page_storage->len-1];
    page_storage->page_array[page_storage->len-1] = EMPTY_VALUE;
    page_storage->len -= 1;
    return popped_page;
}

//complete eviction
void process_evict(sorted_mem_pages* free_memory_pool, sorted_mem_pages* page_storage, unsigned long n_pages_to_keep, unsigned long current_time){
    unsigned long freed_page;

    //printf("\nEVICTING ALL BUT %lu PAGES", n_pages_to_keep);
    //check first to avoid removing an extra page
    if (page_storage->len <= n_pages_to_keep){
        return;
    }

    if (page_storage->len == 0){
        printf("\n\n===========TRYING TO EVIC PROCESS WITH 0 PAGES ALLOCATED\n\n");
        return;
    }
    if (page_storage->len == 1){
        printf("%lu, EVICTED, mem-addresses=[%lu]\n", current_time, page_array_pop_last(page_storage));
        return;
    }
    printf("%lu, EVICTED, mem-addresses=[", current_time);
    while (1){ 
        freed_page = page_array_pop_last(page_storage);
        page_array_insert(free_memory_pool, freed_page);
        if (page_storage->len == n_pages_to_keep){
            printf("%lu]\n", freed_page);
            break;
        }else{
            printf("%lu,", freed_page);
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


//fill initial memory pool with pages (maintain reverse sorted ordering)
sorted_mem_pages* populate_free_memory_pool(unsigned long available_memory){
    sorted_mem_pages* free_memory_pool = construct_mem_pages();
    unsigned long* page_array = malloc((available_memory/4) * sizeof(unsigned long));

    //ensure initial fill is sorted in reverse order
    for (unsigned long i = 0; i < (available_memory/4); i++){ 
        page_array[(available_memory/4) - 1 - i] = i;
    }
    //ready to output
    free_memory_pool->len = available_memory/4;
    free_memory_pool->alloced_len = available_memory/4;
    free_memory_pool->page_array = page_array;
    return free_memory_pool;
}


//***********************************************************************************************
//swapping

unsigned long mem_swap(sorted_mem_pages** mem_hash_table, sorted_mem_pages* free_memory_pool, process_queue* working_queue, unsigned long requesting_process_id, unsigned long pages_required, unsigned long current_time, char memory_manager){
    if (DEBUG){
        printf("\n==================SWAPPING MEM====================\n");
    }

    if (working_queue == NULL){
        printf("\nnot enough memory and no processes in working queue to free!!! i don't have handling for this!\n");
        return 0;
    }

    //printf("\nfree memory len before swap: %lu", free_memory_pool->len);
    //printf("\npages required: %lu", pages_required);

    //how many pages to keep in process we are evicting from
    unsigned long n_pages_to_keep;

    //until enough memory is freed, free all memory allocated single processes
    queue_node* removal_target_process = working_queue->front;
    while (free_memory_pool->len < pages_required && removal_target_process != NULL){
        if (memory_manager != MEM_UNLIMITED){
            //printf("\nchecking if pid %lu has alloced mem we can take...", removal_target_process->value->process_id);
            if (mem_hash_table[removal_target_process->value->process_id]->len > 0){
                //printf(" found memory");
                if (memory_manager == MEM_SWAPPING){
                    n_pages_to_keep = EVICT_ALL;
                }else{
                    n_pages_to_keep = max(mem_hash_table[removal_target_process->value->process_id]->len, pages_required) - pages_required;
                }
                process_evict(free_memory_pool, mem_hash_table[removal_target_process->value->process_id], n_pages_to_keep, current_time);
            }
        }
        removal_target_process = removal_target_process->prev;
    }
    //printf("||");


    //load memory into processs
    unsigned long pages_swapped = 0;
    unsigned long swap_page;
    while (pages_swapped < pages_required){
        swap_page = page_array_pop_last(free_memory_pool);
        page_array_insert(mem_hash_table[requesting_process_id], swap_page);
        pages_swapped += 1;
    }
    return pages_swapped;
}


//***********************************************************************************************
//selects between different memory loading types, returns cost
unsigned long load_memory(process* requesting_process, sorted_mem_pages** mem_hash_table, sorted_mem_pages* free_memory_pool, process_queue* working_queue, unsigned long process_page_req, unsigned long current_time, char memory_manager){
    unsigned long free_page;
    unsigned long cost;
    unsigned long initial_pages_required = (requesting_process->memory_size_req / 4) - mem_hash_table[requesting_process->process_id]->len;
    unsigned long current_pages_required = initial_pages_required;

    //don't need to manage memory when set to unlimited
    if (memory_manager == MEM_UNLIMITED){
        return 0;
    }

    //printf("\nfree memory len before allocing %lu", free_memory_pool->len);
    //load pages from free memory until it's exhausted or the program has enough memory requirements
    //don't add cost as free memory doesn't need any disk stuff
    while (free_memory_pool->len > 0 && current_pages_required > 0){
        free_page = page_array_pop_last(free_memory_pool);
        if (DEBUG){
            printf("\nLOADING PAGE #%lu FROM FREE MEMORY POOL", free_page);
            printf(" -AFTER POP, FREE POOL CONTAINS:");
            for (unsigned long cc = 0; cc < free_memory_pool->len; cc++){
                printf("%lu->", free_memory_pool->page_array[cc]);
            }
            printf("END");
        }

        page_array_insert(mem_hash_table[requesting_process->process_id], free_page);
        current_pages_required -= 1;
    }

    //if requirements could be fulfilled from free memory, return early
    if (current_pages_required == 0){
        cost = initial_pages_required * LOADING_COST;
        return cost;
    }

    //swap memory as required
    if (memory_manager == MEM_VIRTUAL){
        current_pages_required = min(4, process_page_req - mem_hash_table[requesting_process->process_id]->len);
        //printf("\nprocess requires %lu pages", current_pages_required);
    }
    cost =  mem_swap(mem_hash_table, free_memory_pool, working_queue, requesting_process->process_id, current_pages_required, current_time, memory_manager) * LOADING_COST;
    return cost;
}


//***********************************************************************************************
//scheduler helper, adds arrived processes to working queue
void enqueue_arrived_processes(unsigned long current_time, process_queue* working_queue, process_queue* incoming_process_queue){
    if (incoming_process_queue->len == 0){
        if (DEBUG){
            printf("no new processes have arrived\n");
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

//***********************************************************************************************
//helper functions to output and perform simple prep work
void process_running_print(unsigned long current_time, sorted_mem_pages** mem_hash_table, sorted_mem_pages* free_memory_pool, unsigned long memory_size, process* cur_process, unsigned long load_cost, char memory_manager){
    if (memory_manager == MEM_UNLIMITED){
        printf("%lu, RUNNING, id=%lu, remaining-time=%lu\n", current_time, cur_process->process_id, cur_process->job_time);
    }else{
        //100% - %free = %used
        unsigned long mem_use_percent = 100 - (100 * free_memory_pool->len)/ (memory_size/4); //+ memory_size/4 - 1) to ensure percent is rounded up
        printf("%lu, RUNNING, id=%lu, remaining-time=%lu, load-time=%lu, mem-usage=%lu%%, mem-addresses=[", current_time, cur_process->process_id, cur_process->job_time, load_cost, mem_use_percent);
        for (unsigned long i = mem_hash_table[cur_process->process_id]->len; i > 0; i--){
            if (i - 1 == 0){
                printf("%lu]\n", mem_hash_table[cur_process->process_id]->page_array[i-1]);
            }else{
                printf("%lu,", mem_hash_table[cur_process->process_id]->page_array[i-1]);
            }
        }
    }
}


//***********************************************************************************************
//first come first served scheduler

void first_come_first_served(process_queue* incoming_process_queue, sorted_mem_pages* free_memory_pool, sorted_mem_pages** mem_hash_table, unsigned long memory_size, char memory_manager){
    unsigned long current_time = 0;
    unsigned long load_cost = 0;

    process_queue* working_queue = construct_queue();
    enqueue_arrived_processes(current_time, working_queue, incoming_process_queue);

    process* cur_process = queue_dequeue(working_queue);
    if (memory_manager != MEM_UNLIMITED){
        load_cost = load_memory(cur_process, mem_hash_table, free_memory_pool, working_queue, cur_process->memory_size_req/4, current_time, memory_manager);
        //apply loading penalty
        cur_process->job_time += (cur_process->memory_size_req/4 - mem_hash_table[cur_process->process_id]->len);
    }
    process_running_print(current_time, mem_hash_table, free_memory_pool, memory_size, cur_process, load_cost, memory_manager);
    current_time += load_cost;

    while (1){
        //simulate time spent running job
        current_time += cur_process->job_time;
        cur_process->job_time = 0;

        //evict if necessary
        if (memory_manager != MEM_UNLIMITED){
            process_evict(free_memory_pool, mem_hash_table[cur_process->process_id], EVICT_ALL, current_time);
        }
        
        //prepare for next iteration and update
        enqueue_arrived_processes(current_time, working_queue, incoming_process_queue);
        printf("%lu, FINISHED, id=%lu, proc-remaining=%lu\n", current_time, cur_process->process_id, working_queue->len);

        //wait for new processes if required
        if (working_queue->len == 0){
            if (DEBUG){
                printf("waiting for new process...\n");
            }
            //all processes complete
            if (incoming_process_queue->len == 0){
                if (DEBUG){
                    printf("all processes complete...\n");
                }
                return;
            }
            //wait for next process
            current_time += incoming_process_queue->front->value->time_arrived - current_time;
            enqueue_arrived_processes(current_time, working_queue, incoming_process_queue);
        }

        //set next process. apparently running starts at point pages begin to be loaded, rather than afterwards BUT all the pages that will be loaded have to be output when it starts running ? very fucky:/
        cur_process = queue_dequeue(working_queue);
        //load required pages
        if (memory_manager != MEM_UNLIMITED){
            load_cost = load_memory(cur_process, mem_hash_table, free_memory_pool, working_queue, cur_process->memory_size_req/4, current_time, memory_manager);
            //apply loading penalty
            cur_process->job_time += (cur_process->memory_size_req/4 - mem_hash_table[cur_process->process_id]->len);
        }
        process_running_print(current_time, mem_hash_table, free_memory_pool, memory_size, cur_process, load_cost, memory_manager);
        current_time += load_cost; //load cost is always 0 for mem_unlimited
    }
}


//***********************************************************************************************
//round robin scheduler



void round_robin(process_queue* incoming_process_queue, sorted_mem_pages* free_memory_pool, sorted_mem_pages** mem_hash_table, unsigned long quantum, unsigned long memory_size, char memory_manager){
    unsigned long current_time = 0;
    unsigned long load_cost = 0;
    process* cur_process;

    //used to store processes that have actually arrived
    process_queue* working_queue = construct_queue();
    enqueue_arrived_processes(current_time, working_queue, incoming_process_queue);

    //set first process and load memory
    cur_process = queue_dequeue(working_queue);
    if (memory_manager != MEM_UNLIMITED){
        load_cost = load_memory(cur_process, mem_hash_table, free_memory_pool, working_queue, cur_process->memory_size_req/4, current_time, memory_manager);
        //apply loading penalty
        cur_process->job_time += (cur_process->memory_size_req/4 - mem_hash_table[cur_process->process_id]->len);
    }
    process_running_print(current_time, mem_hash_table, free_memory_pool, memory_size, cur_process, load_cost, memory_manager);
    current_time += load_cost;

    //run until no processes remain
    while (1==1){
        //if job will be finished in this quantum
        if (cur_process->job_time <= quantum){
            //update time values and enqueue new processes
            current_time += cur_process->job_time;
            cur_process->job_time = 0;
            enqueue_arrived_processes(current_time, working_queue, incoming_process_queue);
            if (memory_manager != MEM_UNLIMITED){
                process_evict(free_memory_pool, mem_hash_table[cur_process->process_id], EVICT_ALL, current_time);
            }
            printf("%lu, FINISHED, id=%lu, proc-remaining=%lu\n", current_time, cur_process->process_id, working_queue->len);

            //if no jobs can be swapped to, simulate waiting for the next job to arrive
            if (working_queue->len == 0){
                if (DEBUG){
                    printf("nothing in working queue...\n");
                }
                
                //wait for processes that haven't yet arrived
                if (incoming_process_queue->len != 0){
                    if (DEBUG){
                        printf("waiting for processes...\n");
                    }
                    
                    current_time += (incoming_process_queue->front->value->time_arrived - current_time);
                    //enqueue arrived processes
                    enqueue_arrived_processes(current_time, working_queue, incoming_process_queue);
                }else{
                    //if no jobs exist to work on and no jobs will arrive in the future, return
                    if (DEBUG){
                        printf("all processes complete\n");
                    }                    
                    return;
                }
            }
            //load next process
            cur_process = queue_dequeue(working_queue);

            //apparently running starts before memory is loaded ZZZZZZZ
            //printf("%lu, RUNNING, id=%lu, remaining-time=%lu\n", current_time, cur_process->process_id, cur_process->job_time);
            if (memory_manager != MEM_UNLIMITED){
                //need to do integration work on this
                load_cost = load_memory(cur_process, mem_hash_table, free_memory_pool, working_queue, cur_process->memory_size_req/4, current_time, memory_manager);
                //apply loading penalty
                cur_process->job_time += (cur_process->memory_size_req/4 - mem_hash_table[cur_process->process_id]->len);
            }
            process_running_print(current_time, mem_hash_table, free_memory_pool, memory_size, cur_process, load_cost, memory_manager);
            current_time += load_cost;
            

        //if job won't be finished this quantum, shuffle it to the back and select a new job
        }else{
            //update time values and enqueue new processes
            current_time += quantum;
            cur_process->job_time -= quantum;
            enqueue_arrived_processes(current_time, working_queue, incoming_process_queue);
            //place process at back of queue and announce return from suspension
            queue_enqueue(working_queue, cur_process);
            cur_process = queue_dequeue(working_queue);
            if (memory_manager != MEM_UNLIMITED){
                load_cost = load_memory(cur_process, mem_hash_table, free_memory_pool, working_queue, cur_process->memory_size_req/4, current_time, memory_manager);
                //apply loading penalty
                cur_process->job_time += (cur_process->memory_size_req/4 - mem_hash_table[cur_process->process_id]->len);
            }
            process_running_print(current_time, mem_hash_table, free_memory_pool, memory_size, cur_process, load_cost, memory_manager);
            current_time += load_cost;
        }
        
    }
}


//***********************************************************************************************
//



//***********************************************************************************************
//hidden to actual assignment, stores incoming data with having to constantly do IO
process_queue* load_processes(char* filename){
    //open file
    FILE* file = fopen(filename, "r");

    //if file open failed
    if (file == NULL){
        if (DEBUG){
            printf("null file, returning...\n");
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

    //get control args
    int opt;
    unsigned long quantum = 0;
    unsigned long memory_size = 0;
    char memory_manager = '\0';
    char scheduling_algorithm = '\0';
    char* filename = NULL;
    while ((opt = getopt(argc, argv, "f:a:m:s:q:")) != -1){
        if (DEBUG){
            printf("new param - ");
        }
        
        if (opt == 'f'){
            filename = optarg;
            if (DEBUG){
                printf("filename: %s\n", filename);
            }
            
        }
        if (opt == 'a'){
            scheduling_algorithm = optarg[0];
            if (DEBUG){
                printf("scheduling algorithm: %c\n", scheduling_algorithm);
            }
            
        }
        if (opt == 'm'){
            memory_manager = optarg[0];
            if (DEBUG){
                printf("allocator type: %c\n", memory_manager);
            }
            
        }
        if (opt == 's'){
            memory_size = strtoul(optarg, NULL, 0);
            if (DEBUG){
                printf("memory size: %lu\n", memory_size);
            }
            
        }
        if (opt == 'q'){
            quantum = strtoul(optarg, NULL, 0);
            if (DEBUG){
                printf("quantum: %lu\n", quantum);
            }
            
        }
        if (opt == '?'){
            if (DEBUG){
                printf("1 or more args not recognised... returning\n");
            }
            
            return 1;
        }
    }

    if (scheduling_algorithm == '\0' || memory_manager == '\0' || filename == NULL){
        if (DEBUG){
            printf("a required arg wasn't set... returning\n");
        }
        
        return 1;
    }

    //read data
    if (DEBUG){
        printf("attempting to read file: \"%s\"\n", filename);
    }
    
    process_queue* incoming_process_queue = load_processes(filename);
    if (DEBUG){
        print_queue(incoming_process_queue);
    }
    

    //handle errors during read
    if (incoming_process_queue->len == 0){
        if (DEBUG){
            printf("error during file read... returning...\n");
        }
        return 1;
    }

    //initialize memory pool and mem hashtable if they will be used
    //set to 1 to avoid crashes when populating memory
    if (memory_size == 0){
        memory_size = 1;
    }
    sorted_mem_pages* free_memory_pool = populate_free_memory_pool(memory_size);
    sorted_mem_pages** mem_usage_table = create_hash_table();

    
    //run round robin scheduler with unlimited memory
    if (scheduling_algorithm == SCHEDULER_RR){
        round_robin(incoming_process_queue, free_memory_pool, mem_usage_table, quantum, memory_size, memory_manager);
    }
    if (scheduling_algorithm == SCHEDULER_FCFS){
        first_come_first_served(incoming_process_queue, free_memory_pool, mem_usage_table, memory_size, memory_manager);
    }
    if (scheduling_algorithm == SCHEDULER_CUSTOM){
        if (DEBUG){
            printf("custom scheduler is not yet implemented yet\n");
        }
    }
    
    return 0;
}