
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <limits.h>

typedef unsigned long long int addr64_t;

int main() 
{
      /* Trace file contents */
    addr64_t addr_trace; // 64 bit memory address
    char access_trace;   // can be w or r for write/read
    int i= 0;

    while (fscanf(stdin, " %c %llx", &access_trace, &addr_trace) == 2)
        printf("\r\n cnt %d %c %lld \r\n", i++, access_trace, addr_trace); 
    return 0; 
} 