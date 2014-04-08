/**
 * some programe wide marcos and stuff
 */
#ifndef __MAIN_H__
#define __MAIN_H__

/**
 * Programm Name
 */
#define PROG_NAME "xpminer-v1.0"

/**
 * Prototypes for all stuctures
 */
typedef struct MinerArgs   MinerArgs;
typedef struct MiningStats MiningStats;
typedef struct Opts        Opts;
typedef struct BlockHeader BlockHeader;
typedef struct TestParams  TestParams;
typedef struct SieveStats  SieveStats;
typedef struct Sieve       Sieve;
typedef struct PrimeTable  PrimeTable;

/**
 * programm versions
 */
#define VERSION_MAJOR 0
#define VERSION_MINOR 8
#define VERSION_EXT "RC1"

/**
 * the default pool fee (xolominer)
 */
#define DEFAULT_POOL_FEE 3

/**
 * the default number of threads to use
 */
#define DEFAULT_NUM_THREADS 4

/**
 * default number of sieve extensions
 */ 
#define DEFAULT_SIEVE_EXTENSIONS 9

/**
 * default number of sieve percentage
 */
#define DEFAULT_SIEVE_PRIMES 30000

/**
 * default sieve size
 */
#define DEFAULT_SIEVE_SIZE 4000000u

/**
 * the default numper of primes the block header hash
 * should be divisible by
 * (begining by 2)
 */
#define DEFAULT_NUM_PRIMES_IN_HASH 4
#define MAX_NUM_PRIMES_IN_HASH 9

/**
 * the default number of primes the primorial
 * should be divisible by
 */
#define DEFAULT_NUM_PRIMES_IN_PRIMORIAL 5

/**
 * the default chain length to mine
 */
#define DEFAULT_CHAIN_LENGTH 10

/**
 * the default number of bytes to preocess in chae while sieveing
 */
#define DEFAULT_CACHE_BITS 224000

/**
 * the maximum chain length to deal with (word recors is 17)
 */
#define MAX_CHAIN_LENGTH 32

/**
 * default intervall (in seconds) to output status informations
 */
#define DEFAULT_STATS_INTERVAL 60

/**
 * default minimum chainlength whicht will be submitted as share to the pool
 */
#define DEFAULT_POOL_SHARE 7

/**
 * to define program wide globals
 */
#ifndef EXTERN
#define EXTERN extern
#endif

/* indecates wether the programm should shutdown */
EXTERN char running;

/* global options */
EXTERN Opts opts;

/**
 * thread to output statsus informations
 */
void *stats_thread(void *thread_args);

/**
 * main thread which starts the miner threads
 * and outputs stats
 */
void main_thread(MinerArgs *args);


/**
 * include all other headers
 */
#include "verbose.h"
#include "options.h"
#include "net.h"
#include "block.h"
#include "prime-table.h"
#include "prime-tests.h"
#include "sieve.h"
#include "tests.h"
#include "autotune.h"

/**
 * args for the mining threads
 */
struct MinerArgs {
  Sieve    sieve;
  uint32_t id;
  uint32_t n_threads;
  char     new_work;
  char     mine;
  pthread_mutex_t mutex;
};

/* the differnt stats of mining */
#define MINING_WAIT    0
#define MINING_START   1
#define MINING_STARTED 2
#define MINING_STOPPED 3

#endif /* __MAIN_H__ */
