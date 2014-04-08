/**
 * Code to store program wider parameters and
 * parse comandline options
 */
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <stdio.h>
#include <openssl/sha.h>
#include <math.h>

#include "main.h"

/**
 * macros to access the readed opts
 */
#define LICENSE              0
#define POOL_FEE             1
#define POOL_IP              2
#define POOL_PORT            3
#define POOL_USER            4
#define POOL_PASSWORD        5
#define NUM_THREADS          6
#define MINER_ID             7
#define SIEVE_EXTENSIONS     8
#define SIEVE_PRIMES         9
#define SIEVE_SIZE          10
#define PRIMES_IN_HASH      11
#define PRIMES_IN_PRIMORIAL 12
#define CHAIN_LENGTH        13
#define CACHE_BITS          14
#define VERBOSE             15
#define STATS_INTERVAL      16
#define POOL_SHARE          17
#define USE_FIRST_HALF      18
#define QUIET               19

/**
 * the avilabe comand line options
 */
static struct option long_options[] = {
  { "license",             no_argument,       0, LICENSE             },
  { "pool-fee",            required_argument, 0, POOL_FEE            },
  { "pool-ip",             required_argument, 0, POOL_IP             },
  { "pool-port",           required_argument, 0, POOL_PORT           },
  { "pool-user",           required_argument, 0, POOL_USER           },
  { "pool-pwd",            required_argument, 0, POOL_PASSWORD       },
  { "num-threads",         required_argument, 0, NUM_THREADS         },
  { "miner-id",            required_argument, 0, MINER_ID            },
  { "sieve-extensions",    required_argument, 0, SIEVE_EXTENSIONS    },
  { "sieve-primes",        required_argument, 0, SIEVE_PRIMES        },
  { "sieve-size",          required_argument, 0, SIEVE_SIZE          },
  { "primes-in-hash",      required_argument, 0, PRIMES_IN_HASH      },
  { "primes-in-primorial", required_argument, 0, PRIMES_IN_PRIMORIAL },
  { "chain-length",        required_argument, 0, CHAIN_LENGTH        },
  { "cache-bits",          required_argument, 0, CACHE_BITS          },
  { "verbose",             no_argument      , 0, VERBOSE             },
  { "stats-interval",      required_argument, 0, STATS_INTERVAL      },
  { "pool-share",          required_argument, 0, POOL_SHARE          },
  { "use-first-half",      no_argument      , 0, USE_FIRST_HALF      },
  { "quiet",               no_argument,       0, QUIET               },
  { 0,                     0,                 0, 0                   }
};

/**
 * initialize promgram wide parameters
 */
void init_program_parameters() {

  opts.header = calloc(sizeof(BlockHeader), 1);

  /* init offset to zero */
  opts.time_offset = 0;

  mpz_init(opts.mpz_primorial);
  mpz_init(opts.mpz_fixed_hash_multiplier);

  /* chache bits need to be a multiple of word_bits */
  opts.cache_bits = (opts.cache_bits / word_bits) * word_bits;

  if (opts.cache_bits == 0)
    opts.cache_bits = word_bits;

  /**
   * sive size need to be a multiplie of 2 * cache_bits 
   * (extensions using oly the half array)
   */
  opts.sieve_size = (opts.sieve_size / (2 * opts.cache_bits)) *
                   (2 * opts.cache_bits);

  if (opts.sieve_size == 0)
    opts.sieve_size = 2 * opts.cache_bits;

  opts.sieve_words = (opts.sieve_size / word_bits);

  /* make sure enough primes are calculated */
  uint32_t sieve_size = opts.sieve_primes;

  /* n / log(n) is an lower bound for the numbers of primes smaler than n */
  while ((sieve_size / log(sieve_size)) < opts.sieve_primes)
    sieve_size *= 2;

  /* generate the prime table */
  opts.primes = gen_prime_table(sieve_size);

  /* the higest prime index to sieve */
  opts.max_prime_index = opts.sieve_primes;

  /* generate the hash_primorial */
  int_primorial(opts.primes, &opts.hash_primorial, opts.primes_in_hash);


  /* generate the fixed hash multiplier */
  primorial(opts.primes, 
            opts.mpz_fixed_hash_multiplier, 
            opts.primes_in_hash, 
            opts.primes_in_primorial);

  /**
   * make sure fixed_hash_multiplier meets the minimum 
   * while fixed_hash_multiplier < hash_primorial
   *
   * (if the hash is divisible by more primes you have to multiply more
   *  primes for the primorial)
   */
  for (; mpz_cmp_ui(opts.mpz_fixed_hash_multiplier, 
                    opts.hash_primorial) <= 0; opts.primes_in_primorial++) {

    primorial(opts.primes, 
              opts.mpz_fixed_hash_multiplier, 
              opts.primes_in_hash, 
              opts.primes_in_primorial);
  } 

  /**
   * create the inverses of two for all used primes 
   */
  uint32_t i;
  opts.two_inverses = malloc(sizeof(uint32_t) * opts.primes->len);

  opts.int64_arithmetic = UINT32_MAX;

  for (i = 0; i < opts.primes->len; i++) {
    
    opts.two_inverses[i] = (opts.primes->ptr[i] + 1) / 2;
    
    /* set the index after which we have to use 64 bit aritmetic */
    if (opts.int64_arithmetic == UINT32_MAX &&
        UINT32_MAX / opts.two_inverses[i] < opts.primes->ptr[i]) {

      opts.int64_arithmetic = i;
    }
  }

  /** 
   * estimate the sieve percentage 
   * n / log(n) is a good appriximation for the
   * amount of primes till a given number n
   */
  opts.sieve_percentage = (opts.sieve_primes * 100) / 
                          (opts.sieve_size / log(opts.sieve_size));

  /* encrypt the password with sha1 */
  uint32_t *pwd_hash = (uint32_t *) SHA1((unsigned char *) opts.pool_pwd, 
                                         strlen(opts.pool_pwd),
                                         NULL);

  opts.pool_pwd = (char *) calloc(sizeof(char), 17);

  /* shorten the sha1 password (taken form xolominer) */
  sprintf(opts.pool_pwd, 
          "%08x%08x", 
          pwd_hash[0] ^ pwd_hash[1] ^ pwd_hash[4],
          pwd_hash[2] ^ pwd_hash[3] ^ pwd_hash[4]);

  /* init sieve globals */
  init_sieve_globals();

  opts.start_time = time(NULL);
}

/**
 * free promgram wide parameters on shutdown
 */
void free_opts() {
  
  mpz_clear(opts.mpz_primorial);
  mpz_clear(opts.mpz_fixed_hash_multiplier);

  free(opts.primes->ptr);
  free(opts.two_inverses);
  free(opts.primes);
  free(opts.pool_pwd);
  free(opts.header);

  free_sieve_globals();
}


/**
 * read the comand line options into a Opts struct
 */
void init_opts(int argc, char *argv[]) {

  memset(&opts, 0, sizeof(Opts));
 
  int opt       = 0;
  int opt_index = 0;
 
  /* loop */
  for (;;) {
   
    opt = getopt_long_only(argc, argv, "", long_options, &opt_index);
    
    /* detect end of options */
    if (opt == -1)
      break;
    
    /* parse the option */
    switch (opt) {
    
      case LICENSE:
        print_license();
        break;

      case POOL_FEE:
        opts.pool_fee = atoi(optarg);
        break;
    
      case POOL_IP:
        opts.pool_ip = optarg;
        break;
    
      case POOL_PORT:
        opts.pool_port = atoi(optarg);
        break;
    
      case POOL_USER:
        opts.pool_user = optarg;
        break;
    
      case POOL_PASSWORD:
        opts.pool_pwd = optarg;
        break;

      case NUM_THREADS:
        opts.num_threads = atoi(optarg);
        break;

      case MINER_ID:
        opts.miner_id = atoi(optarg);
        break;

      case SIEVE_EXTENSIONS:
        opts.sieve_extensions = atoi(optarg);
        break;

      case SIEVE_PRIMES:
        opts.sieve_primes = atoi(optarg);
        break;

      case SIEVE_SIZE:
        opts.sieve_size = atoi(optarg);
        break;

      case PRIMES_IN_HASH:
        opts.primes_in_hash = atoi(optarg);
        break;

      case PRIMES_IN_PRIMORIAL:
        opts.primes_in_primorial = atoi(optarg);
        break;

      case CHAIN_LENGTH:
        opts.chain_length = atoi(optarg);
        break;

      case CACHE_BITS:
        opts.cache_bits = atoi(optarg);
        break;

      case VERBOSE:
        opts.verbose = 1;
        break;

      case STATS_INTERVAL:
        opts.stats_interval = atoi(optarg);
        break;

      case POOL_SHARE:
        opts.pool_share = atoi(optarg);
        break;

      case USE_FIRST_HALF:
        opts.use_first_half = 1;
        break;

      case QUIET:
        opts.quiet = 1;
        break;
    }
  }

  /* exit if not all neccesary options ar given */
  if (opts.pool_ip   == NULL || 
      opts.pool_port == 0    || 
      opts.pool_user == NULL) {

    print_help();
  }

  /* set default options */
  if (opts.pool_fee <= 0 || opts.pool_fee > 100)
    opts.pool_fee = DEFAULT_POOL_FEE;

  if (opts.pool_pwd == NULL)
    opts.pool_pwd = "very insecure!!!";

  if (opts.num_threads <= 0)
    opts.num_threads = DEFAULT_NUM_THREADS;

  if (opts.sieve_extensions <= 0)
    opts.sieve_extensions = DEFAULT_SIEVE_EXTENSIONS;

  if (opts.sieve_primes <= 0)
    opts.sieve_primes = DEFAULT_SIEVE_PRIMES;

  if (opts.sieve_size <= 0)
    opts.sieve_size = DEFAULT_SIEVE_SIZE;

  if (opts.primes_in_hash <= 0)
    opts.primes_in_hash = DEFAULT_NUM_PRIMES_IN_HASH;

  if (opts.primes_in_hash > MAX_NUM_PRIMES_IN_HASH)
    opts.primes_in_hash = MAX_NUM_PRIMES_IN_HASH;

  if (opts.primes_in_primorial <= 0)
    opts.primes_in_primorial = DEFAULT_NUM_PRIMES_IN_PRIMORIAL;

  if (opts.chain_length <= 0)
    opts.chain_length = DEFAULT_CHAIN_LENGTH;

  if (opts.cache_bits <= 0)
    opts.cache_bits = DEFAULT_CACHE_BITS;

  if (opts.stats_interval <= 0)
    opts.stats_interval = DEFAULT_STATS_INTERVAL;

  if (opts.pool_share <= 0)
    opts.pool_share = DEFAULT_POOL_SHARE;


  /* init promgram wide parameters */
  init_program_parameters();
}
