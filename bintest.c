

#include "stdlib.h"
#include "stdio.h"

int main(){
unsigned long val = 1;
unsigned long data[6] = {10, 8, 7, 5, 3, 2};
unsigned long mid = 9999999; //default just for checking
unsigned long low = 0;
unsigned long high = 6;
while (low != high){
	 mid = low/2 + high/2;
         if (data[mid] >= val){ //use >= rather than <= to search reverse ordered array 
        	 low = mid + 1;
         }else{
        	 high = mid;
         }
}

printf("low: %lu", low);
return 0;
}
