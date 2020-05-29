//includes
#include "stdio.h"
#include "stdlib.h"

//defines
#define MAX_NUM_PROCESSES 1000

//process information struct
typedef struct Process{
    int time_arrived;
    int process_id;
    int memory_size_req;
    int job_time;
} process;

//process helper function
void print_process(process target_process){
    printf("%d %d %d %d\n", target_process.time_arrived, target_process.process_id, 
    target_process.memory_size_req, target_process.job_time);
}

//hidden to actual assignment, stores incoming data with having to constantly do IO
process* create_incoming_process_array(char* filename, int* new_len){
    process* process_array = calloc(MAX_NUM_PROCESSES, sizeof(process));
    if (process_array == NULL){
        printf("realloc failed!!! program should prob exit so add code for it");
    }
    FILE* file = fopen(filename, "r");
    
    //if file open failed
    if (file == NULL){
        printf("null file, returning...");
        return 0;
    }

    //read processes file into buffer
    int offset = 0;
    for (;fscanf(file,"%d %d %d %d\r\n", &process_array[offset].time_arrived, 
            &process_array[offset].process_id, &process_array[offset].memory_size_req, 
            &process_array[offset].job_time) == 4;offset += 1){
    }
    *new_len = offset;
    return process_array;
}

int main(int argc, char** argv){
    //check correct args are passed
    if (argc != 6){
        printf("incorrect number of arguments, had %d, expected 5\n returning...", argc-1);
        return 1;
    }

    //read data
    printf("attempting to read file: \"%s\"\n", argv[1]);
    int* process_ar_len = malloc(sizeof(int));
    process* process_array = create_incoming_process_array(argv[1], process_ar_len);

    //handle error
    if (*process_ar_len == 0){
        printf("error during file read... returning...");
        return 1;
    }else{
        for (int i = 0; i < *process_ar_len; i++){
            print_process(process_array[i]);
        }
    }
    return 0;
}