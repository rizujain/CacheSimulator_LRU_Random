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
static char repl;

static int numberSets;
static int indexBits;
static int offsetBits;
static int tagBits;

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

static void print_help(void)
{
    printf("The program ./simulate_cache takes the following arguments: \r\n");
    printf("nk: the capacity of the cache in kilobytes (an int) \n");
    printf("assoc: the associativity of the cache (an int) \n");
    printf("blocksize: the size of a single cache block in bytes (an int) \n");
    printf("repl: the replacement policy (a char); 'l' means LRU, 'r' means random. \n");
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
        repl = 1; // Random replacement policy
    else
        repl = 0; // LRU replacement policy (default)

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

/*
 * Main function
 */
int main(int argc, const char *argv[])
{
    int ret = 0;
    cache_struct mycache;
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
    printf("repl %d \r\n", repl);
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

    /* entities to count hits and misses */
    int TSTAMP = 0; //value for LRU
    int empty = -1; //index of empty space
    int H = 0;      //is there a hit
    int E = 0;      //is there an eviction
    int count_hits = 0;
    int count_misses = 0;
    int count_evictions = 0;

    /* Trace file contents */
    addr64_t addr_trace;
    char access_trace; // can be w or r for write/read

    /* Trace File Handling */

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
            if (access_trace != 'I')
            {
                //calculate address tag and set index
                addr64_t addr_tag = addr_trace >> (offsetBits + indexBits);
                unsigned long long temp = addr_trace << (tagBits);
                unsigned long long setid = temp >> (tagBits + offsetBits);
                set_cache set = mycache.set[setid];
                int low = INT_MAX;

                for (int e = 0; e < blocksize; e++)
                {
                    if (set.block[e].valid == 1)
                    {
                        // CHANGED ORDER: look for hit before eviction candidates
                        if (set.block[e].tag == addr_tag)
                        {
                            count_hits++;
                            H = 1;
                            set.block[e].timestamp = TSTAMP;
                            TSTAMP++;
                        }
                        // CHANGED WHOLE ELSE: look for oldest for eviction.
                        else if (set.block[e].timestamp < low)
                        {
                            low = set.block[e].timestamp;
                            toEvict = e;
                        }
                    }
                    // CHANGED: if we haven't yet found an empty, mark one that we found.
                    else if (empty == -1)
                    {
                        empty = e;
                    }
                }

                //if we have a miss
                if (H != 1)
                {
                    count_misses++;
                    //if we have an empty line
                    if (empty > -1)
                    {
                        set.block[empty].valid = 1;
                        set.block[empty].tag = addr_tag;
                        set.block[empty].timestamp = TSTAMP;
                        TSTAMP++;
                    }
                    //if the set is full we need to evict
                    else if (empty < 0)
                    {
                        E = 1;
                        set.block[toEvict].tag = addr_tag;
                        set.block[toEvict].timestamp = TSTAMP;
                        TSTAMP++; // CHANGED: increment TSTAMP here too
                        count_evictions++;
                    }
                }
                //if the instruction is M, we will always get a hit
                if (access_trace == 'M')
                {
                    count_hits++;
                }

                empty = -1;
                H = 0;
                E = 0;
            }
        }
    }
    printf("hits: %d   misses: %d   evictions: %d\n", count_hits, count_misses, count_evictions);

    return 0;
}
