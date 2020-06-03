#include "process.h"
#include "stdio.h"

void print_process(process* target_process){
    printf("%lu %lu %lu %lu\n", target_process->time_arrived, target_process->process_id, target_process->memory_size_req, target_process->job_time);
}