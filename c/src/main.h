/**
 * some programe wide marcos and stuff
 */
#ifndef __MAIN_H__
#define __MAIN_H__

/**
 * Prototypes for all stuctures
 */
typedef struct MinerArgs MinerArgs;
typedef struct MiningStats MiningStats;
typedef struct Opts Opts;
typedef struct BlockHeader BlockHeader;
typedef struct TestParams TestParams;
typedef struct SieveStats SieveStats;
typedef struct Sieve Sieve;
typedef struct PrimeTable PrimeTable;

/**
 * programm versions
 */
#define VERSION_MAJOR 0
#define VERSION_MINOR 8
#define VERSION_EXT "RC1"

/**
 * default number of sieve extensions
 */ 
#define DEFAULT_SIEVE_EXTENSIONS 9

/**
 * default number of sieve percentage
 */
#define DEFAULT_SIEVE_PERCENTAGE 10

/**
 * default sieve size
 */
#define DEFAULT_SIEVE_SIZE 1000000u

/**
 * the default numper of primes the block header hash
 * should be divisible by
 * (begining by 2)
 */
#define DEFAULT_NUM_PRIMES_IN_HASH 4

/**
 * the default chain length to mine
 */
#define DEFAULT_CHAIN_LENGTH 7

/**
 * the default number of bytes to preocess in chae while sieveing
 */
#define DEFAULT_CACHE_BYTES 224000

/**
 * the maximum chain length to deal with (word recors is 17)
 */
#define MAX_CHAIN_LENGTH 32

/**
 * include all other headers
 */
#include "options.h"
#include "net.h"
#include "block.h"
#include "prime-table.h"
#include "prime-tests.h"
#include "sieve.h"
#include "verbose.h"

/**
 * args for the mining threads
 */
struct MinerArgs {
  Sieve    sieve;
  Opts     *opts;
  uint32_t id;
  uint32_t n_threads;
  char     new_work;
  char     *mine;
  pthread_mutex_t mutex;
};

#endif /* __MAIN_H__ */
