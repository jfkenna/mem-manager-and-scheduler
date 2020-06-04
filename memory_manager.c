#include "memory_manager.h"
#include "config.h"
#include "stdio.h"

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
//swap pages from processes into requesting process, places residual memory into free memory pool

unsigned long mem_swap(sorted_mem_pages** mem_hash_table, sorted_mem_pages* free_memory_pool, 
    process_queue* working_queue, unsigned long requesting_process_id, unsigned long pages_required, 
    unsigned long current_time, char memory_manager, sorted_mem_pages* temp_evicted_pages){

    if (working_queue == NULL){
        return 0;
    }

    //number of pages that should remain after the eviction is complete
    unsigned long n_pages_to_keep;
    unsigned long total_pages_required = pages_required;

    //evict memory from processes until enough memory has been freed
    queue_node* removal_target_process = working_queue->front;
    if (memory_manager == MEM_CUSTOM){
        removal_target_process = working_queue->back;
    }
    while (pages_required > 0 && removal_target_process != NULL){
        if (memory_manager != MEM_UNLIMITED){
            if (mem_hash_table[removal_target_process->value->process_id]->len > 0){
                //evict all memory if using swapping, take only as much as necessary if using other management methods
                if (memory_manager == MEM_SWAPPING){
                    n_pages_to_keep = EVICT_ALL;
                    pages_required -= mem_hash_table[removal_target_process->value->process_id]->len - n_pages_to_keep;
                }else{
                    n_pages_to_keep = max(mem_hash_table[removal_target_process->value->process_id]->len, pages_required) - pages_required;
                    pages_required -= mem_hash_table[removal_target_process->value->process_id]->len - n_pages_to_keep;
                }
                process_evict(free_memory_pool, mem_hash_table[removal_target_process->value->process_id], 
                    n_pages_to_keep, current_time, temp_evicted_pages);
            }
        }
        if (memory_manager == MEM_CUSTOM){
            removal_target_process = removal_target_process->next;
        }else{
            removal_target_process = removal_target_process->prev;
        }
    }


    //load memory into processs
    unsigned long pages_swapped = 0;
    unsigned long swap_page;
    while (pages_swapped < total_pages_required){
        swap_page = page_array_pop_last(free_memory_pool);
        page_array_insert(mem_hash_table[requesting_process_id], swap_page);
        pages_swapped += 1;
    }
    return pages_swapped;
}


//***********************************************************************************************
//loads memory for a process and returns total loading cost
unsigned long load_memory(process* requesting_process, sorted_mem_pages** mem_hash_table, 
    sorted_mem_pages* free_memory_pool, process_queue* working_queue, unsigned long process_page_req, 
    unsigned long current_time, char memory_manager, sorted_mem_pages* temp_evicted_pages){

    unsigned long free_page;
    unsigned long cost;
    unsigned long initial_pages_required = (requesting_process->memory_size_req / 4) - mem_hash_table[requesting_process->process_id]->len;
    unsigned long current_pages_required = initial_pages_required;

    if (memory_manager == MEM_UNLIMITED){
        return 0;
    }

    //load pages from free memory until the free pool is exhausted or the process has enough memory
    while (free_memory_pool->len > 0 && current_pages_required > 0){
        free_page = page_array_pop_last(free_memory_pool);
        page_array_insert(mem_hash_table[requesting_process->process_id], free_page);
        current_pages_required -= 1;
    }
    

    //if memory requirements were met by taking from the free pool, return early
    if (((memory_manager == MEM_VIRTUAL || memory_manager == MEM_CUSTOM) 
        && mem_hash_table[requesting_process->process_id]->len >= 4) || current_pages_required == 0){

        cost = (initial_pages_required - current_pages_required) * LOADING_COST;
        return cost;
    }

    //evict pages from other processes in order to meet memory requirements 
    unsigned long early_swap = 0;
    unsigned long pages_to_get = (initial_pages_required - mem_hash_table[requesting_process->process_id]->len);
    if (memory_manager == MEM_VIRTUAL || memory_manager == MEM_CUSTOM){
        early_swap = initial_pages_required - current_pages_required;
        pages_to_get = min(4, process_page_req) - mem_hash_table[requesting_process->process_id]->len;
    }
    cost =  (early_swap + mem_swap(mem_hash_table, 
        free_memory_pool, working_queue, requesting_process->process_id, 
        pages_to_get, current_time, memory_manager, temp_evicted_pages)) * LOADING_COST;
    return cost;
}
