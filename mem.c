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


//hidden to actual assignment, stores incoming data with having to constantly do IO
process* create_incoming_process_array(char* filename){
    process* process_array = calloc(MAX_NUM_PROCESSES, sizeof(process));
    FILE* file = fopen(filename, "r");
    
    //file open failed
    if (file == NULL){
        printf("null file, returning...");
        return NULL;
    }
    //read processes file into buffer, BUGGED
    int offset = 0;
    while (fscanf(file,"%d %d %d %d\r\n", &process_array[offset].time_arrived, 
            &process_array[offset].process_id, &process_array[offset].memory_size_req, 
            &process_array[offset].job_time) != EOF){
        offset += 1;
        printf("offset: %d, ", offset);
        printf("%d %d %d %d\n", process_array[offset].time_arrived, process_array[offset].process_id,
         process_array[offset].memory_size_req, process_array[offset].job_time);  
    }
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
    process* process_array = create_incoming_process_array(argv[1]);
    if (process_array == NULL){
        printf("error during file read... returning...");
        return 1;
    }
    printf("end");
    return 0;
}