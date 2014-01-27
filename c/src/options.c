/**
 * Code to store program wider parameters and
 * parse comandline options
 */
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <stdio.h>
#include <openssl/sha.h>

#include "main.h"

/**
 * macros to access the readed opts
 */
#define POOLFEE       1
#define POOLIP        2
#define POOLPORT      3
#define POOLUSER      4
#define POOLPASSWORD  5
#define GENPROCLIMIT  6
#define MINERID       7
#define NSIEVEEXT     8
#define NSIEVEPERCENT 9
#define SIEVESIZE     10
#define NPRIMESINHASH 11
#define NPRIMESINPRIM 12
#define CHAINLENGTH   13
#define CACHEBYTES    14
#define VERBOSE       15
#define STATSINTERVAL 16
#define POOLSHARE     17

/**
 * the avilabe comand line options
 */
static struct option long_options[] = {
  { "poolfee",       required_argument, 0, POOLFEE       },
  { "poolip",        required_argument, 0, POOLIP        },
  { "poolport",      required_argument, 0, POOLPORT      },
  { "pooluser",      required_argument, 0, POOLUSER      },
  { "poolpassword",  required_argument, 0, POOLPASSWORD  },
  { "genproclimit",  required_argument, 0, GENPROCLIMIT  },
  { "minerid",       required_argument, 0, MINERID       },
  { "nsieveext",     required_argument, 0, NSIEVEEXT     },
  { "nsievepercent", required_argument, 0, NSIEVEPERCENT },
  { "sievesize",     required_argument, 0, SIEVESIZE     },
  { "nprimesinhash", required_argument, 0, NPRIMESINHASH },
  { "nprimesinprim", required_argument, 0, NPRIMESINPRIM },
  { "chainlength",   required_argument, 0, CHAINLENGTH   },
  { "cachebits",    required_argument, 0, CACHEBYTES    },
  { "verbose",       no_argument      , 0, VERBOSE       },
  { "statsinterval", required_argument, 0, STATSINTERVAL },
  { "poolshare",     required_argument, 0, POOLSHARE     },
  { 0,               0,                 0, 0             }
};

/**
 * initialize promgram wide parameters
 */
static inline void init_program_parameters(Opts *opts) {

  opts->header = calloc(sizeof(BlockHeader), 1);

  mpz_init(opts->mpz_hash_primorial);
  mpz_init(opts->mpz_primorial);
  mpz_init(opts->mpz_primorial_primes);

  /* generate the prime table */
  opts->primes = gen_prime_table(opts->sievesize);

  /* generate the hash_primorial */
  mpz_set_ui(opts->mpz_hash_primorial, 1);
  primorial(opts->primes,
            opts->mpz_hash_primorial, 
            0, 
            opts->n_primes_in_hash);

  /* generate the primorial primes */
  mpz_set_ui(opts->mpz_primorial_primes, 1);
  primorial(opts->primes, 
            opts->mpz_primorial_primes, 
            opts->n_primes_in_hash, 
            opts->n_primes_in_primorial);

  /* calculate the higest prime index to sieve */
  opts->max_prime_index = (opts->primes->len * opts->n_sieve_percentage) / 100;

  /* chachebits need to be a multiple of word_bits */
  opts->cachebits = (opts->cachebits / word_bits) * word_bits;

  if (opts->cachebits == 0)
    opts->cachebits = word_bits;

  /**
   * sive size need to be a multiplie of 2 * cachebits 
   * (extensions using oly the half array)
   */
  opts->sievesize = (opts->sievesize / (2 * opts->cachebits)) *
                    (2 * opts->cachebits);

  if (opts->sievesize == 0)
    opts->sievesize = 2 * opts->cachebits;

  opts->sieve_words = (opts->sievesize / word_bits);


  /* highes used index = max_prime_index - 1 */
  while (opts->primes->ptr[opts->max_prime_index - 1] >= // TODO cacl sievesize depending on cachesize hire
         (uint32_t) opts->cachebits) {
    
    opts->max_prime_index--;
  }

  /* encrypt the password with sha1 */
  uint32_t *pwd_hash = (uint32_t *) SHA1((unsigned char *) opts->poolpassword, 
                                         strlen(opts->poolpassword),
                                         NULL);

  opts->poolpassword = calloc(sizeof(char), 17);

  sprintf(opts->poolpassword, 
          "%08x%08x", 
          pwd_hash[0] ^ pwd_hash[1] ^ pwd_hash[4],
          pwd_hash[2] ^ pwd_hash[3] ^ pwd_hash[4]);
}

/**
 * free promgram wide parameters on shutdown
 */
void free_opts(Opts *opts) {
  
  mpz_clear(opts->mpz_hash_primorial);
  mpz_clear(opts->mpz_primorial);
  mpz_clear(opts->mpz_primorial_primes);

  free(opts->primes->ptr);
  free(opts->primes);
  free(opts->poolpassword);
  free(opts->header);
}


/**
 * read the comand line options into a Opts struct
 * TODO validate given args
 */
Opts *init_opts(int argc, char *argv[]) {

  Opts *opts = malloc(sizeof(Opts));
  memset(opts, 0, sizeof(Opts));
 
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
    
      case POOLFEE:
        opts->poolfee = atoi(optarg);
        break;
    
      case POOLIP:
        opts->poolip = optarg;
        break;
    
      case POOLPORT:
        opts->poolport = atoi(optarg);
        break;
    
      case POOLUSER:
        opts->pooluser = optarg;
        break;
    
      case POOLPASSWORD:
        opts->poolpassword = optarg;
        break;

      case GENPROCLIMIT:
        opts->genproclimit = atoi(optarg);
        break;

      case MINERID:
        opts->minerid = atoi(optarg);
        break;

      case NSIEVEEXT:
        opts->n_sieve_extensions = atoi(optarg);
        break;

      case NSIEVEPERCENT:
        opts->n_sieve_percentage = atoi(optarg);
        break;

      case SIEVESIZE:
        opts->sievesize = atoi(optarg);
        break;

      case NPRIMESINHASH:
        opts->n_primes_in_hash = atoi(optarg);
        break;

      case NPRIMESINPRIM:
        opts->n_primes_in_primorial = atoi(optarg);
        break;

      case CHAINLENGTH:
        opts->chain_length = atoi(optarg);
        break;

      case CACHEBYTES:
        opts->cachebits = atoi(optarg);
        break;

      case VERBOSE:
        opts->verbose = 1;
        break;

      case STATSINTERVAL:
        opts->stats_interval = atoi(optarg);
        break;

      case POOLSHARE:
        opts->poolshare = atoi(optarg);
        break;
    }
  }

  /* exit if not all neccesary options ar given */
  if (opts->poolip == NULL || 
      opts->poolport == 0  || 
      opts->pooluser == NULL) {

    printf("-poolip, -poolport and -pooluser are required options!\n");
    exit(EXIT_FAILURE);
  }

  /* set default options */
  if (opts->poolfee <= 0 || opts->poolfee > 100)
    opts->poolfee = 3;

  if (opts->poolpassword == NULL)
    opts->poolpassword = "very insecure!!!";

  if (opts->genproclimit <= 0)
    opts->genproclimit = 1;

  if (opts->n_sieve_extensions <= 0)
    opts->n_sieve_extensions = DEFAULT_SIEVE_EXTENSIONS;

  if (opts->n_sieve_percentage <= 0 || opts->n_sieve_percentage > 100)
    opts->n_sieve_percentage = DEFAULT_SIEVE_PERCENTAGE;

  if (opts->sievesize <= 0)
    opts->sievesize = DEFAULT_SIEVE_SIZE;

  if (opts->n_primes_in_hash <= 0)
    opts->n_primes_in_hash = DEFAULT_NUM_PRIMES_IN_HASH;

  if (opts->n_primes_in_primorial <= 0)
    opts->n_primes_in_primorial = DEFAULT_NUM_PRIMES_IN_PRIMORIAL;

  if (opts->chain_length <= 0)
    opts->chain_length = DEFAULT_CHAIN_LENGTH;

  if (opts->cachebits <= 0)
    opts->cachebits = DEFAULT_CACHE_BITS;

  if (opts->stats_interval <= 0)
    opts->stats_interval = DEFAULT_STATS_INTERVAL;

  if (opts->poolshare <= 0)
    opts->poolshare = DEFAULT_POOL_SHARE;


  /* init promgram wide parameters */
  init_program_parameters(opts);

  return opts;
}
