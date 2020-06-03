//includes
#include "stdio.h"
#include "stdlib.h"
#include "unistd.h" //option processing
#include "string.h"

#include "config.h"
#include "process.h"
#include "queue.h"
#include "sorted_pages.h"
#include "memory_manager.h"
#include "schedulers.h"
#include "stats.h"


//***********************************************************************************************
//creates a queue of incoming data that the simulation queries to determine if anything has arrived
process_queue* load_processes(char* filename){
    //open file
    FILE* file = fopen(filename, "r");

    //if file open failed
    if (file == NULL){
        return NULL;
    }
    
    //load processes
    process_queue* incoming_process_queue = construct_queue();
    process* new_process;
    for (new_process= malloc(sizeof(process)); fscanf(file,"%lu %lu %lu %lu\r\n", &new_process->time_arrived, 
            &new_process->process_id, &new_process->memory_size_req, 
            &new_process->job_time) == 4;new_process = malloc(sizeof(process))){
        new_process->initial_job_time = new_process->job_time;
        //enqueue process
        queue_enqueue(incoming_process_queue, new_process);
    }
    free(new_process);
    fclose(file);
    return incoming_process_queue;
}


//***********************************************************************************************
//runs simulation

int main(int argc, char** argv){
    //disable print buffer
    setvbuf(stdout, NULL, _IONBF, 0);

    int opt;
    unsigned long quantum = 0;
    unsigned long memory_size = 0;
    char memory_manager = '\0';
    char scheduling_algorithm = '\0';
    char* filename = NULL;

    //read command line args
    while ((opt = getopt(argc, argv, "f:a:m:s:q:")) != -1){
        if (opt == 'f'){
            filename = optarg;
        }
        if (opt == 'a'){
            scheduling_algorithm = optarg[0];
        }
        if (opt == 'm'){
            memory_manager = optarg[0];
        }
        if (opt == 's'){
            memory_size = strtoul(optarg, NULL, 0);  
        }
        if (opt == 'q'){
            quantum = strtoul(optarg, NULL, 0); 
        }
        if (opt == '?'){
            return 1;
        }
    }

    if (scheduling_algorithm == '\0' || memory_manager == '\0' || filename == NULL){  
        return 1;
    }

    //read data
    process_queue* incoming_process_queue = load_processes(filename);
    

    //stop if errors occur during read
    if (incoming_process_queue->len == 0){
        return 1;
    }

    //initialize memory pool,memory hashtable, and stats
    if (memory_size == 0){
        memory_size = 1; //set to 1, as program relies on the memory pool having size >= 1
    }
    sorted_mem_pages* free_memory_pool = populate_free_memory_pool(memory_size);
    sorted_mem_pages** mem_usage_table = create_hash_table();
    stats* overall_stats = construct_stats();

    //select and run scheduler with provided algorithms
    if (scheduling_algorithm == SCHEDULER_RR){
        round_robin(incoming_process_queue, free_memory_pool, mem_usage_table, quantum, memory_size, memory_manager, overall_stats);
    }
    if (scheduling_algorithm == SCHEDULER_FCFS){
        sequential_scheduler(incoming_process_queue, free_memory_pool, mem_usage_table, memory_size, memory_manager, overall_stats, SCHEDULER_FCFS);
    }
    if (scheduling_algorithm == SCHEDULER_CUSTOM){
        sequential_scheduler(incoming_process_queue, free_memory_pool, mem_usage_table, memory_size, memory_manager, overall_stats, SCHEDULER_CUSTOM);
    }

    //free before exit
    free_sorted_mem_pages(free_memory_pool);
    for (unsigned long i = 0; i < MAX_NUM_PROCESSES; i++){
        free_sorted_mem_pages(mem_usage_table[i]);
    }
    free(mem_usage_table);
    free_stats(overall_stats);
    free(incoming_process_queue);
    return 0;
}