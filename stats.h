#ifndef STATS_H
#define STATS_H
#include "process.h"
typedef struct Stats stats;
struct Stats{
    unsigned long* endtimes;
    unsigned long turnaround_aggregate;
    double overhead_aggregate;
    double max_overhead;
    unsigned long n_processes;
};

stats* construct_stats();
void free_stats(stats* target);
void update_stats(stats* overall_stats, process* completed_process, unsigned long current_time);
void output_final_stats(stats* overall_stats, unsigned long current_time);
#endif