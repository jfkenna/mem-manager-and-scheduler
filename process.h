#ifndef PROCESS_H
#define PROCESS_H
typedef struct Process process;
struct Process{
    unsigned long time_arrived;
    unsigned long process_id;
    unsigned long memory_size_req;
    unsigned long job_time;
    unsigned long initial_job_time;
};

void print_process(process* target_process);
#endif
