#include "schedulers.h"
#include "process.h"
#include "stdio.h"
#include "stdlib.h"

//***********************************************************************************************
//Some of the lines in this file are quite long
//and may be difficult to read if you are using a small screen.
//I attempted to decompose them into multiple lines using \ 
//but found that  the resulting lines were less readable.
//Apologies : (

    
//***********************************************************************************************
//updates working queue with any arrived processes
void enqueue_arrived_processes(unsigned long current_time, process_queue* working_queue, 
    process_queue* incoming_process_queue){

    if (incoming_process_queue->len == 0){
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
//ouputs RUNNING data
void process_running_print(unsigned long current_time, sorted_mem_pages** mem_hash_table, 
    sorted_mem_pages* free_memory_pool, unsigned long memory_size, process* cur_process, 
    unsigned long load_cost, char memory_manager){

    if (memory_manager == MEM_UNLIMITED){
        printf("%lu, RUNNING, id=%lu, remaining-time=%lu\n", current_time, cur_process->process_id, cur_process->job_time);
    }else{
        //100% - %free = %used
        unsigned long mem_use_percent = 100 - (100 * free_memory_pool->len)/ (memory_size/4); 
        printf("%lu, RUNNING, id=%lu, remaining-time=%lu, load-time=%lu, mem-usage=%lu%%, mem-addresses=[", 
        current_time, cur_process->process_id, cur_process->job_time, load_cost, mem_use_percent);
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

void sequential_scheduler(process_queue* incoming_process_queue, sorted_mem_pages* free_memory_pool,
    sorted_mem_pages** mem_hash_table, unsigned long memory_size, char memory_manager, 
    stats* overall_stats, char scheduler){

    unsigned long current_time = 0;
    unsigned long load_cost = 0;

    //store pages evicted at the same time but in seperate evict events
    sorted_mem_pages* temp_evicted_pages = construct_mem_pages();

    process_queue* working_queue = construct_queue();
    enqueue_arrived_processes(current_time, working_queue, incoming_process_queue);
    process* cur_process;

    if (scheduler == SCHEDULER_FCFS){
        cur_process = queue_dequeue(working_queue);
    }
    if (scheduler == SCHEDULER_CUSTOM){
        cur_process = queue_dequeue_shortest_job(working_queue);
    }
    
    if (memory_manager != MEM_UNLIMITED){
        load_cost = load_memory(cur_process, mem_hash_table, free_memory_pool, 
            working_queue, cur_process->memory_size_req/4, current_time, 
            memory_manager, temp_evicted_pages);

        //apply loading penalty
        cur_process->job_time += (cur_process->memory_size_req/4 - mem_hash_table[cur_process->process_id]->len);

        //print evicts
        print_evict(temp_evicted_pages, current_time);
    }
    process_running_print(current_time, mem_hash_table, free_memory_pool, 
        memory_size, cur_process, load_cost, memory_manager);
    current_time += load_cost;

    while (1){
        //simulate time spent running job
        current_time += cur_process->job_time;
        cur_process->job_time = 0;

        //evict if necessary
        if (memory_manager != MEM_UNLIMITED){
            process_evict(free_memory_pool, mem_hash_table[cur_process->process_id], 
                EVICT_ALL, current_time, temp_evicted_pages);

            print_evict(temp_evicted_pages, current_time);
        }
        
        //prepare for next iteration and update
        enqueue_arrived_processes(current_time, working_queue, incoming_process_queue);
        update_stats(overall_stats, cur_process, current_time);
        printf("%lu, FINISHED, id=%lu, proc-remaining=%lu\n", current_time, 
            cur_process->process_id, working_queue->len);

        //wait for new processes if required
        if (working_queue->len == 0){

            //return once all processes are complete
            if (incoming_process_queue->len == 0){
                output_final_stats(overall_stats, current_time);
                free(cur_process);
                free_sorted_mem_pages(temp_evicted_pages);
                free(working_queue);
                return;
            }
            //wait for next process
            current_time += incoming_process_queue->front->value->time_arrived - current_time;
            enqueue_arrived_processes(current_time, working_queue, incoming_process_queue);
        }

        free(cur_process);
        //if FCFS grab the next process from queue
        if (scheduler == SCHEDULER_FCFS){
            cur_process = queue_dequeue(working_queue);
        }
        //if shortest first, grab the shortest runtime process from queue
        if (scheduler == SCHEDULER_CUSTOM){
            cur_process = queue_dequeue_shortest_job(working_queue);
        }
        
        //load required pages
        if (memory_manager != MEM_UNLIMITED){
            load_cost = load_memory(cur_process, mem_hash_table, free_memory_pool, working_queue, 
                cur_process->memory_size_req/4, current_time, memory_manager, temp_evicted_pages);

            //apply loading penalty
            cur_process->job_time += (cur_process->memory_size_req/4 - mem_hash_table[cur_process->process_id]->len);

            //print evicts
            print_evict(temp_evicted_pages, current_time);
        }
        process_running_print(current_time, mem_hash_table, free_memory_pool, 
            memory_size, cur_process, load_cost, memory_manager);

        current_time += load_cost;
    }
}


//***********************************************************************************************
//round robin scheduler
void round_robin(process_queue* incoming_process_queue, sorted_mem_pages* free_memory_pool, 
    sorted_mem_pages** mem_hash_table, unsigned long quantum, unsigned long memory_size, 
    char memory_manager, stats* overall_stats){

    unsigned long current_time = 0;
    unsigned long load_cost = 0;
    process* cur_process;

    //store pages evicted at the same time but in seperate evict events
    sorted_mem_pages* temp_evicted_pages = construct_mem_pages();

    //used to store processes that have actually arrived
    process_queue* working_queue = construct_queue();
    enqueue_arrived_processes(current_time, working_queue, incoming_process_queue);

    //set first process and load memory
    cur_process = queue_dequeue(working_queue);
    if (memory_manager != MEM_UNLIMITED){
        load_cost = load_memory(cur_process, mem_hash_table, free_memory_pool, working_queue,
            cur_process->memory_size_req/4, current_time, memory_manager, temp_evicted_pages);

        //apply loading penalty
        cur_process->job_time += (cur_process->memory_size_req/4 - mem_hash_table[cur_process->process_id]->len);

        //print evicts
        print_evict(temp_evicted_pages, current_time);
    }
    process_running_print(current_time, mem_hash_table, free_memory_pool, memory_size, 
        cur_process, load_cost, memory_manager);

    current_time += load_cost;

    //run until no processes remain
    while (1){
        //if job will be finished in this quantum
        if (cur_process->job_time <= quantum){
            //update time values and enqueue new processes
            current_time += cur_process->job_time;
            cur_process->job_time = 0;
            enqueue_arrived_processes(current_time, working_queue, incoming_process_queue);
            if (memory_manager != MEM_UNLIMITED){
                process_evict(free_memory_pool, mem_hash_table[cur_process->process_id], 
                    EVICT_ALL, current_time, temp_evicted_pages);

                //print evictions
                print_evict(temp_evicted_pages, current_time);
            }
            update_stats(overall_stats, cur_process, current_time);
            printf("%lu, FINISHED, id=%lu, proc-remaining=%lu\n", current_time, 
                cur_process->process_id, working_queue->len);

            free(cur_process);
            //if no jobs can be swapped to, simulate waiting for the next job to arrive
            if (working_queue->len == 0){

                //wait for processes that haven't yet arrived
                if (incoming_process_queue->len != 0){
                    current_time += (incoming_process_queue->front->value->time_arrived - current_time);
                    //enqueue arrived processes
                    enqueue_arrived_processes(current_time, working_queue, incoming_process_queue);
                }else{
                    //if no jobs exist to work on and no jobs will arrive in the future, return
                    output_final_stats(overall_stats, current_time);
                    free_sorted_mem_pages(temp_evicted_pages);
                    free(working_queue);               
                    return;
                }
            }
            cur_process = queue_dequeue(working_queue);

            if (memory_manager != MEM_UNLIMITED){
                load_cost = load_memory(cur_process, mem_hash_table, free_memory_pool, 
                    working_queue, cur_process->memory_size_req/4, current_time, 
                    memory_manager, temp_evicted_pages);

                //apply loading penalty
                cur_process->job_time += (cur_process->memory_size_req/4 - mem_hash_table[cur_process->process_id]->len);

                //print evictions
                print_evict(temp_evicted_pages, current_time);
            }
            process_running_print(current_time, mem_hash_table, free_memory_pool, memory_size, 
                cur_process, load_cost, memory_manager);
            current_time += load_cost;
            

        //if job won't be finished this quantum, shuffle process to the back of the queue and select a new job
        }else{
            //update time values and enqueue new processes
            current_time += quantum;
            cur_process->job_time -= quantum;
            enqueue_arrived_processes(current_time, working_queue, incoming_process_queue);
            //place process at back of queue and announce return from suspension
            queue_enqueue(working_queue, cur_process);
            cur_process = queue_dequeue(working_queue);
            if (memory_manager != MEM_UNLIMITED){
                load_cost = load_memory(cur_process, mem_hash_table, free_memory_pool, 
                    working_queue, cur_process->memory_size_req/4, current_time,
                     memory_manager, temp_evicted_pages);

                //apply loading penalty
                cur_process->job_time += (cur_process->memory_size_req/4 - mem_hash_table[cur_process->process_id]->len);
                print_evict(temp_evicted_pages, current_time);
            }
            process_running_print(current_time, mem_hash_table, free_memory_pool, 
                memory_size, cur_process, load_cost, memory_manager);

            current_time += load_cost;
        }
        
    }
}
