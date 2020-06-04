#include "stats.h"
#include "queue.h"
#include "sorted_pages.h"
#include "memory_manager.h"
#include "config.h"
void sequential_scheduler(process_queue* incoming_process_queue, sorted_mem_pages* free_memory_pool, 
    sorted_mem_pages** mem_hash_table, unsigned long memory_size, char memory_manager, 
    stats* overall_stats, char scheduler);
    
void round_robin(process_queue* incoming_process_queue, sorted_mem_pages* free_memory_pool, 
    sorted_mem_pages** mem_hash_table, unsigned long quantum, unsigned long memory_size, 
    char memory_manager, stats* overall_stats);
