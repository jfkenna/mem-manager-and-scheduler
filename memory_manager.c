//***********************************************************************************************
//utility functions
#include "memory_manager.h"
#include "config.h"
#include "stdio.h"

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
//swapping

unsigned long mem_swap(sorted_mem_pages** mem_hash_table, sorted_mem_pages* free_memory_pool, process_queue* working_queue, unsigned long requesting_process_id, unsigned long pages_required, unsigned long current_time, char memory_manager, sorted_mem_pages* temp_evicted_pages){
    if (DEBUG){
        printf("\n==================SWAPPING MEM====================\n");
    }

    if (working_queue == NULL){
        printf("\nnot enough memory and no processes in working queue to free!!! i don't have handling for this!\n");
        return 0;
    }

    //printf("free memory len before swap: %lu\n", free_memory_pool->len);
    //printf("pages required: %lu\n", pages_required);

    //how many pages to keep in process we are evicting from
    unsigned long n_pages_to_keep;
    unsigned long total_pages_required = pages_required;

    //until enough memory is freed, free all memory allocated single processes
    queue_node* removal_target_process = working_queue->front;
    if (memory_manager == MEM_CUSTOM){
        removal_target_process = working_queue->back;
    }
    while (pages_required > 0 && removal_target_process != NULL){
        if (memory_manager != MEM_UNLIMITED){
            //printf("\nchecking if pid %lu has alloced mem we can take...", removal_target_process->value->process_id);
            if (mem_hash_table[removal_target_process->value->process_id]->len > 0){
                //printf(" found %lu pages", mem_hash_table[removal_target_process->value->process_id]->len);
                if (memory_manager == MEM_SWAPPING){
                    n_pages_to_keep = EVICT_ALL;
                    pages_required -= mem_hash_table[removal_target_process->value->process_id]->len - n_pages_to_keep;
                }else{
                    n_pages_to_keep = max(mem_hash_table[removal_target_process->value->process_id]->len, pages_required) - pages_required;
                    pages_required -= mem_hash_table[removal_target_process->value->process_id]->len - n_pages_to_keep;
                }
                process_evict(free_memory_pool, mem_hash_table[removal_target_process->value->process_id], n_pages_to_keep, current_time, temp_evicted_pages);
            }
        }
        //printf("free mem pages after evict: %lu\n", free_memory_pool->len);
        if (memory_manager == MEM_CUSTOM){
            removal_target_process = removal_target_process->next;
        }else{
            removal_target_process = removal_target_process->prev;
        }
        //printf("pid - %lu", removal_target_process->value->process_id);
    }


    //load memory into processs
    unsigned long pages_swapped = 0;
    unsigned long swap_page;
    //printf("pages required %lu\n", total_pages_required); 
    //printf("free mem pages %lu\n", free_memory_pool->len);
    while (pages_swapped < total_pages_required){
        //printf("%lu\n", free_memory_pool->len);
        swap_page = page_array_pop_last(free_memory_pool);
        page_array_insert(mem_hash_table[requesting_process_id], swap_page);
        //printf("process now has %lu pages\n", mem_hash_table[requesting_process_id]->len);
        pages_swapped += 1;
    }
    return pages_swapped;
}

//***********************************************************************************************
//selects between different memory loading types, returns cost
unsigned long load_memory(process* requesting_process, sorted_mem_pages** mem_hash_table, sorted_mem_pages* free_memory_pool, process_queue* working_queue, unsigned long process_page_req, unsigned long current_time, char memory_manager, sorted_mem_pages* temp_evicted_pages){
    unsigned long free_page;
    unsigned long cost;
    unsigned long initial_pages_required = (requesting_process->memory_size_req / 4) - mem_hash_table[requesting_process->process_id]->len;
    unsigned long current_pages_required = initial_pages_required;

    //printf("\nTOT PAGES REQUIRED %lu", (requesting_process->memory_size_req / 4));
    //printf("\nCUR PAGES %lu", mem_hash_table[requesting_process->process_id]->len);
    //printf("\nINIT PAGES REQUIRED %lu", initial_pages_required);
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
    //printf("||process has len %lu\n", mem_hash_table[requesting_process->process_id]->len);
    if (((memory_manager == MEM_VIRTUAL || memory_manager == MEM_CUSTOM) && mem_hash_table[requesting_process->process_id]->len >= 4) || current_pages_required == 0){
        //printf("as process now has len %lu, returning", mem_hash_table[requesting_process->process_id]->len);
        cost = (initial_pages_required - current_pages_required) * LOADING_COST;
        return cost;
    }

    //swap memory as required
    //idk if this early swap section even works, as it doesn't come up in the test cases as far as i can tell
    unsigned long early_swap = 0;
    unsigned long pages_to_get = (initial_pages_required - mem_hash_table[requesting_process->process_id]->len);
    if (memory_manager == MEM_VIRTUAL || memory_manager == MEM_CUSTOM){
        early_swap = initial_pages_required - current_pages_required;
        //printf("early swap size of %lu\n", early_swap);
        pages_to_get = min(4, process_page_req) - mem_hash_table[requesting_process->process_id]->len;
        //printf("pages to get: %lu", pages_to_get);
    }
    cost =  (early_swap + mem_swap(mem_hash_table, free_memory_pool, working_queue, requesting_process->process_id, pages_to_get, current_time, memory_manager, temp_evicted_pages)) * LOADING_COST;
    return cost;
}
