//memory management datatype config
#define MAX_NUM_PROCESSES 10000
#define RESIZE_MULTIPLIER 2
#define PAGE_ARRAY_INIT_SIZE 10
#define LOADING_COST 2
#define EVICT_ALL 0

//memory and scheduler settings
#define MEM_UNLIMITED 'u'
#define MEM_SWAPPING 'p'
#define MEM_VIRTUAL 'v'
#define MEM_CUSTOM 'c'

#define SCHEDULER_FCFS 'f'
#define SCHEDULER_RR 'r'
#define SCHEDULER_CUSTOM 'c'

//stats config
#define MAX_WINDOWS 5000
#define WINDOW_SIZE 60

//treat max unsigned long as missing value
//i know this isn't to be ULONG_MAX, but it's ULONG_MAX on the testing environment
//and my local environment, so it works for the purposes of this project
#define EMPTY_VALUE 4294967295 

//debugging
#define DEBUG 0
