#include "sorted_pages.h"
#include "stdio.h"
#include "stdlib.h"
#include "config.h"


//***********************************************************************************************
//Some of the lines in this file are quite long
//and may be difficult to read if you are using a small screen.
//I attempted to decompose them into multiple lines using \ 
//but found that  the resulting lines were less readable.
//Apologies : (

//***********************************************************************************************
//ordered page constructor / freer
sorted_mem_pages* construct_mem_pages(){
    sorted_mem_pages* new_pages = malloc(sizeof(sorted_mem_pages));
    new_pages->len = 0;
    new_pages->alloced_len = 0;
    new_pages->page_array = NULL;
    return new_pages;
}

void free_sorted_mem_pages(sorted_mem_pages* target){
    if (target->page_array != NULL){
        free(target->page_array);
    }
    free(target);
}


//***********************************************************************************************
//insert page number while maintaining reverse sorted order
void page_array_insert(sorted_mem_pages* page_storage, unsigned long page_number){
    //set initial values if it's the first insertion
    if (page_storage->len == 0){
        if (page_storage->alloced_len == 0){
            page_storage->page_array = malloc(PAGE_ARRAY_INIT_SIZE * sizeof(unsigned long));
            page_storage->alloced_len = PAGE_ARRAY_INIT_SIZE;
        }
        page_storage->page_array[0] = page_number;
        page_storage->len = 1;
    }else{
        //=================================================================================
        //the following binary search for insertion location is a modified form of code obtained from
        //https://stackoverflow.com/questions/24868637/inserting-in-to-an-ordered-array-using-binary-search
        unsigned long mid = 0;
        unsigned long low = 0;
        //len is used rather than len -1 to ensure search includes the spot 1 past the last element of the array
        unsigned long high = page_storage->len;
        while (low != high){
            mid = low/2 + high/2;
            //use >= rather than <= to search reverse ordered array 
            if (page_storage->page_array[mid] >= page_number){ 
                low = mid + 1;
            }else{
                high = mid;
            }
        }
        unsigned long insert_index = low;
        //=============================end modified code===================================

        //if we don't have space to store our new data, reallocate and insert
        if (page_storage->alloced_len < 1 + page_storage->len){  
            //expand allocation, fill with empty values
            unsigned long* new_storage_array = malloc((page_storage->len * RESIZE_MULTIPLIER) * sizeof(unsigned long));
            page_storage->alloced_len = page_storage->len * RESIZE_MULTIPLIER;
            for (unsigned long writeover = 0; writeover < page_storage->alloced_len; writeover++){
                new_storage_array[writeover] = EMPTY_VALUE;
            }

            unsigned long insert_offset = 0;
            for (unsigned long i = 0; i < page_storage->len; i++){
                if (i == insert_index){
                    insert_offset = 1;
                }
                
                new_storage_array[i+insert_offset] = page_storage->page_array[i]; 
            }
            new_storage_array[insert_index] = page_number;
            free(page_storage->page_array);
            page_storage->page_array = new_storage_array;
        }else{
            //if no reallocation is required

            //shift over values to make room for new addition
            unsigned long j = page_storage->len-1;
            while (j >= insert_index){
                page_storage->page_array[j+1] = page_storage->page_array[j];
                //avoid overflow from unsigned variable j falling to -1
                if (j == 0){
                    break;
                }else{
                    j -= 1;
                }
            }
            //insert
            page_storage->page_array[insert_index] = page_number;
        }
        page_storage->len += 1;
    }
}


//***********************************************************************************************
//remove page with smallest page number, as data is sorted by page number
unsigned long page_array_pop_last(sorted_mem_pages* page_storage){
    unsigned long popped_page = page_storage->page_array[page_storage->len-1];
    page_storage->page_array[page_storage->len-1] = EMPTY_VALUE;
    page_storage->len -= 1;
    return popped_page;
}


//***********************************************************************************************
//evict all but N pages from a memory array
void process_evict(sorted_mem_pages* free_memory_pool, sorted_mem_pages* page_storage, 
    unsigned long n_pages_to_keep, unsigned long current_time, 
    sorted_mem_pages* temp_evicted_pages){

    unsigned long freed_page;

    //check first to avoid removing an extra page
    if (page_storage->len <= n_pages_to_keep){
        return;
    }

    if (page_storage->len == 0){
        return;
    }
    
    while (1){ 
        freed_page = page_array_pop_last(page_storage);
        page_array_insert(free_memory_pool, freed_page);
        page_array_insert(temp_evicted_pages, freed_page);
        if (page_storage->len == n_pages_to_keep){
            return;
        }
    }
}


//***********************************************************************************************
//eviction output
void print_evict(sorted_mem_pages* temp_evicted_pages, unsigned long current_time){
    if (temp_evicted_pages->len == 0){
        return;
    }

    unsigned long freed_page;
    printf("%lu, EVICTED, mem-addresses=[", current_time);
    while (1){
        freed_page = page_array_pop_last(temp_evicted_pages);
        if (temp_evicted_pages->len == 0){
            printf("%lu]\n", freed_page);
            return;
        }
        printf("%lu,", freed_page);
    }
}


//***********************************************************************************************
//hash table constructor. closer to a lookup table, as the hashing function is f(x)=x
sorted_mem_pages** create_hash_table(){
    sorted_mem_pages** mem_hash_table = malloc((MAX_NUM_PROCESSES) * sizeof(sorted_mem_pages));
    for (unsigned long i = 0; i < MAX_NUM_PROCESSES; i++){
        mem_hash_table[i] = construct_mem_pages();
    }
    return mem_hash_table;
}


//***********************************************************************************************
//fill free memory pool with pages
sorted_mem_pages* populate_free_memory_pool(unsigned long available_memory){
    sorted_mem_pages* free_memory_pool = construct_mem_pages();
    unsigned long* page_array = malloc((available_memory/4) * sizeof(unsigned long));

    //ensure initial fill is sorted in reverse order
    for (unsigned long i = 0; i < (available_memory/4); i++){ 
        page_array[(available_memory/4) - 1 - i] = i;
    }
    free_memory_pool->len = available_memory/4;
    free_memory_pool->alloced_len = available_memory/4;
    free_memory_pool->page_array = page_array;
    return free_memory_pool;
}