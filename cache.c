/* Author
    Name:   Rizu Jain
    UIN:    430000753
    Email:  rizujain@tamu.edu

    This file is a part of HW2 submission
    For CSCE 614 Fall 2019
    Department of Computer Science & Engineering
    Texas A&M University, College Station.
*/

/* Standard Header files */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include <limits.h>

/* Macros */
#define ADDR_SIZE 64U

/* Debug Options */

#ifndef __DEBUG__
#define __DEBUG__

#ifdef DEBUG
#define debug(fmt, ...) fprintf(stderr, fmt, ##__VA_ARGS__)
#else
#define debug(fmt, ...) ((void)0)
#endif

#endif

/* Global Variable Definitions */

static unsigned long int nk;
static int assoc;
static unsigned long int blocksize;
static char repl_policy;

static int numberSets;
static int indexBits;
static int offsetBits;
static int tagBits;

/* entities to count hits and misses */
int empty = -1;    //index of empty space
int flagHit = 0;   //is there a hit
int flagEvict = 0; //is there an eviction
int count_hits = 0;
int count_reads = 0;
int count_misses_write = 0;
int count_misses_read = 0;
int count_evictions = 0;

/* Typedef for storing addresses */
typedef unsigned long long addr64_t;

/* Structure Definitions */

typedef struct
{
    int timestamp;
    int valid;
    addr64_t tag;
} block_cache;

typedef struct
{
    block_cache *block;
} set_cache;

typedef struct
{
    set_cache *set;
} cache_struct;

cache_struct mycache;


static void print_help(void)
{
    printf("The program ./simulate_cache takes the following arguments: \r\n");
    printf("nk: the capacity of the cache in kilobytes (an int) \n");
    printf("assoc: the associativity of the cache (an int) \n");
    printf("blocksize: the size of a single cache block in bytes (an int) \n");
    printf("repl_policy: the replacement policy (a char); 'l' means LRU, 'r' means random. \n");
}

static int parse_cmdline(int argc, const char *argv[])
{
    if (argc < 5)
    {
        printf("ERR! You entered %d arguments !\r\n", argc);
        print_help();
        return -1;
    }

    /* Parse command line parameters */
    nk = strtoul(argv[1], NULL, 0);
    assoc = strtoul(argv[2], NULL, 0);
    blocksize = strtoul(argv[3], NULL, 0);
    if (strcmp(argv[4], "r") == 0)
        repl_policy = 1; // Random replacement policy
    else
        repl_policy = 0; // LRU replacement policy (default)

    /* TODO: Check power of 2 restrictions on input parameters. */

    return 0;
}

void computeBits(void)
{
    /* Compute total number of sets */
    numberSets = (nk * 1024) / (blocksize * assoc);
    debug("Total Number of Sets %d \r\n", numberSets);
    /* Compute numbder of bits for tag, index and offset */
    offsetBits = (round((log(blocksize)) / (log(2))));
    debug("Offset Bits %d \r\n", offsetBits);
    indexBits = (log(numberSets)) / (log(2));
    debug("Index Bits %d \r\n", indexBits);
    tagBits = ADDR_SIZE - (indexBits + offsetBits);
    debug("Tag Bits %d \r\n", tagBits);
}

void printjob(void)
{
    int total_misses = count_misses_write + count_misses_read;
    int total_access = count_hits + total_misses;
    debug("total_access %d \n", total_access);
    debug("count_reads %d\n", count_reads );
    debug("count_writes %d\n",total_access - count_reads );
    printf("%d %.6f%% %d %.6f%% %d %.6f%%\r\n", total_misses,
           (total_misses * 100 / (double) total_access),
           count_misses_read,
           (count_misses_read * 100 /(double)  count_reads),
           count_misses_write,
           (count_misses_write * 100 /(double) (total_access - count_reads)));
}

/*
 * Main function
 */
int main(int argc, const char *argv[])
{
    int ret = 0;
    int i = 0;
    char *trace_file;

    debug("Welcome to Cache Simulation! \r\n");

    ret = parse_cmdline(argc, argv);
    if (ret < 0)
    {
        printf("%s failed to parse arguments \r\n", argv[0]);
        return ret;
    }

#ifdef DEBUG
    // Verify parsing of commands
    printf("nk %ld \r\n", nk);
    printf("assoc %d \r\n", assoc);
    printf("blocksize %ld \r\n", blocksize);
    printf("repl_policy %d \r\n", repl_policy);
#endif

    /* Initialise a Cache:
    compute parameter values
    and allocate memory for the cache */

    /* Calculate number of sets and cache reg bits */
    computeBits();
    /* memory allocation for sets and block */
    mycache.set = malloc(numberSets * sizeof(set_cache));
    while (i < numberSets)
    {
        mycache.set[i++].block = malloc(blocksize * sizeof(block_cache));
    }

    int timestamp = 0; //value for LRU

    /* Trace file contents */
    addr64_t addr_trace;
    char access_trace; // can be w or r for write/read

    //open the file and read it in
    FILE *traceFile = fopen("trace.txt", "r");
    if (traceFile == NULL)
    {
        printf("no such file.");
        return 0;
    }

    if (traceFile != NULL)
    {
        while (fscanf(traceFile, " %c %llx", &access_trace, &addr_trace) == 2)
        {
            int toEvict = 0; //keeps track of what to evict
            //calculate address tag and set index
            addr64_t addr_tag = addr_trace >> (offsetBits + indexBits);
            unsigned long long temp = addr_trace << (tagBits);
            unsigned long long setid = temp >> (tagBits + offsetBits);
            set_cache set = mycache.set[setid];
            int time_bomb = INT_MAX;

            if (access_trace == 'r')
                count_reads++;

            for (int cnt = 0; cnt < blocksize; cnt++)
            {
                if (set.block[cnt].valid == 1)
                {
                    if (set.block[cnt].tag == addr_tag)
                    {
                        count_hits++;
                        flagHit = 1;
                        set.block[cnt].timestamp = timestamp;
                        timestamp++;
                    }
                    else if (set.block[cnt].timestamp < time_bomb)
                    {
                        time_bomb = set.block[cnt].timestamp;
                        toEvict = cnt;
                    }
                }
                else if (empty == -1)
                {
                    empty = cnt;
                }
            }

            //if we have a miss
            if (flagHit != 1)
            {
                if (access_trace == 'r')
                    count_misses_read++;
                else
                    count_misses_write++;

                //if we have an empty line
                if (empty > -1)
                {
                    set.block[empty].valid = 1;
                    set.block[empty].tag = addr_tag;
                    set.block[empty].timestamp = timestamp;
                    timestamp++;
                }
                //if the set is full we need to evict
                else if (empty < 0)
                {
                    flagEvict = 1;
                    set.block[toEvict].tag = addr_tag;
                    set.block[toEvict].timestamp = timestamp;
                    timestamp++; // CHANGED: increment timestamp here too
                    count_evictions++;
                }
            }

            empty = -1;
            flagHit = 0;
            flagEvict = 0;
        }
    }

    printjob();
    while (i < numberSets)
    {
        free(mycache.set[i++].block);
    }

    return 0;
}
