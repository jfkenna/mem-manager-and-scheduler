#include "stats.h"
#include "stdio.h"
#include "stdlib.h"
#include "config.h"


//***********************************************************************************************
//stats constructor and freer
stats* construct_stats(){
    stats* new_stats = malloc(sizeof(stats));
    new_stats->endtimes = calloc(MAX_WINDOWS, sizeof(unsigned long));
    for (unsigned long i = 0; i < MAX_WINDOWS; i++){
        new_stats->endtimes[i] = 0;
    }
    new_stats->overhead_aggregate = 0;
    new_stats->max_overhead = 0;
    new_stats->turnaround_aggregate = 0;
    new_stats->n_processes = 0;
    return new_stats;
}

void free_stats(stats* target){
    free(target->endtimes);
    free(target);
}


//***********************************************************************************************
//updates stat aggregates, tables, and maximums. called when processes finish running
void update_stats(stats* overall_stats, process* completed_process, unsigned long current_time){
    unsigned long turnaround = current_time - completed_process->time_arrived;
    overall_stats->turnaround_aggregate += turnaround;
    double overhead = (double)turnaround/completed_process->initial_job_time;
    overall_stats->overhead_aggregate += overhead;
    if (overhead > overall_stats->max_overhead){
        overall_stats->max_overhead = overhead;
    }
    overall_stats->endtimes[((current_time + WINDOW_SIZE-1)/WINDOW_SIZE)-1] += 1;
    overall_stats->n_processes += 1;
}


//***********************************************************************************************
//calculates and outputs overall stats for simulation
void output_final_stats(stats* overall_stats, unsigned long current_time){
    
    //if we have travelled past a zero window and later discover a filled window, set min to zero
    unsigned long possible_zero_min = 0;

    //get throughput data
    unsigned long throughput_min = EMPTY_VALUE; //max unsigned long
    unsigned long throughput_max = 0;
    unsigned long throughput_avg = 0;
    unsigned long last_populated_window;

    //calculate window statistics and get averages from aggregates
    for (unsigned long i = 0; i < MAX_WINDOWS; i++){
        if (overall_stats->endtimes[i] != 0){
            last_populated_window = i;
            if (possible_zero_min){
                throughput_min = 0;
            }
        }
        throughput_avg += overall_stats->endtimes[i]; //is actually an aggregate until the loop breaks
        if (overall_stats->endtimes[i] < throughput_min){
            if (overall_stats->endtimes[i] == 0){
                possible_zero_min = 1;
            }else{
                throughput_min = overall_stats->endtimes[i];
            }
        }
        if (overall_stats->endtimes[i] > throughput_max){
            throughput_max = overall_stats->endtimes[i];
        }
    }

    //output stats
    throughput_avg = (throughput_avg + (last_populated_window+1) - 1)/ (last_populated_window+1);
    printf("Throughput %lu, %lu, %lu\n", throughput_avg, throughput_min, throughput_max);
    printf("Turnaround time %lu\n", (overall_stats->turnaround_aggregate + (overall_stats->n_processes - 1)) / overall_stats->n_processes);
    printf("Time overhead %.2lf %.2lf\n", overall_stats->max_overhead, overall_stats->overhead_aggregate/overall_stats->n_processes);
    printf("Makespan %lu\n", current_time);
}
