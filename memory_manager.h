#include "sorted_pages.h"
#include "queue.h"

unsigned long mem_swap(sorted_mem_pages** mem_hash_table, sorted_mem_pages* free_memory_pool, 
    process_queue* working_queue, unsigned long requesting_process_id, unsigned long pages_required, 
    unsigned long current_time, char memory_manager, sorted_mem_pages* temp_evicted_pages);

unsigned long load_memory(process* requesting_process, sorted_mem_pages** mem_hash_table, 
    sorted_mem_pages* free_memory_pool, process_queue* working_queue, unsigned long process_page_req, 
    unsigned long current_time, char memory_manager, sorted_mem_pages* temp_evicted_pages);
