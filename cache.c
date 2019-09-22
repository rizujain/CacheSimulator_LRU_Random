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

/* Structure Definitions */
struct cache_reg
{
    void *check;
};

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
    numberSets = nk / (blocksize * assoc);
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
    struct cache_reg mycache;
    int ret = 0;

    debug("Welcome to Cache Simulation! \r\n");

    ret = parse_cmdline(argc, argv);
    if (ret < 0)
    {
        printf("%s failed to parse arguments\n", argv[0]);
        return ret;
    }

#ifdef DEBUG
    // Verify parsing of commands
    printf("nk %ld \r\n", nk);
    printf("assoc %d \r\n", assoc);
    printf("blocksize %ld \r\n", blocksize);
    printf("repl %d \r\n", repl);
#endif

    computeBits();
    return 0;
}
