/**
 * implementation of the Sieve of Eratosthenes for finding prime chains
 */
#include <sys/time.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <gmp.h>

#include "main.h"

/**
 * The Prime Chain types
 */
#define FIRST_CUNNINGHAM_CHAIN  "1CC"
#define SECOND_CUNNINGHAM_CHAIN "2CC"
#define BI_TWIN_CHAIN           "TWN"

/**
 * access the word in which the given bit-index lays
 */
#define word_at(ary, i) (ary)[(i) / word_bits]

/**
 * returns a word with the given bit index settet
 */
#define bit_word(i) (((sieve_t) 1) << ((i) % word_bits))

/**
 * returns the currnt time in microseconds
 */
static inline uint64_t gettime_usec() {

  struct timeval time;
  if (gettimeofday(&time, NULL) == -1)
    return -1L;

  return time.tv_sec * 1000000L + time.tv_usec;
}

/**
 * prints informations about the sieve
 */
void print_sieve(Sieve *sieve) {

  printf("chain_length:         %d\n"
         "poolshare:            %d\n"
         "candidate_bytes:      %d\n"
         "size:                 %d\n"
         "extensions:           %d\n"
         "layers:               %d\n"
         "max_prime_index:      %d\n"
         "min_prime_index:      %d\n"
         "cache_bits:           %d\n"
         "sieve_words:          %d\n"
         "active:               %d\n",
         (int) sieve->chain_length,
         (int) sieve->poolshare,
         (int) sieve->candidate_bytes,
         (int) sieve->size,
         (int) sieve->extensions,
         (int) sieve->layers,
         (int) sieve->max_prime_index,
         (int) sieve->min_prime_index,
         (int) sieve->cache_bits,
         (int) sieve->sieve_words,
         (int) sieve->active);

  printf("mpz_reminder:         ");
  mpz_out_str(stdout, 10, sieve->mpz_reminder);
  printf("\n");

  printf("mpz_tmp:              ");
  mpz_out_str(stdout, 10, sieve->mpz_tmp);
  printf("\n");

  printf("mpz_hash:             ");
  mpz_out_str(stdout, 10, sieve->mpz_hash);
  printf("\n");

  printf("mpz_hash_primorial:   ");
  mpz_out_str(stdout, 10, sieve->mpz_hash_primorial);
  printf("\n");

  printf("mpz_test_origin:      ");
  mpz_out_str(stdout, 10, sieve->mpz_test_origin);
  printf("\n");

  printf("mpz_primorial_primes: ");
  mpz_out_str(stdout, 10, sieve->mpz_primorial_primes);
  printf("\n");

  printf("mpz_multiplier:       ");
  mpz_out_str(stdout, 10, sieve->mpz_multiplier);
  printf("\n\n");

}

/**
 * sets a new header 
 */
void sieve_set_header(Sieve *sieve, BlockHeader *header) {

  memcpy(&sieve->header, header, sizeof(BlockHeader));
}

/** 
 * reinitilaizes an given sieve TODO make nothing depending on opts, give hader and n_primorial_prims
 */
void reinit_sieve(Sieve *sieve) {

  sieve->active = 1;

  memset(sieve->cc1,   0, sieve->candidate_bytes);
  memset(sieve->cc2,   0, sieve->candidate_bytes);
  memset(sieve->twn,   0, sieve->candidate_bytes);  // TODO should not be neccesary
  memset(sieve->all,   0, sieve->candidate_bytes);  // TODO should not be neccesary

  /* for cc1 and cc2 chains, for each layer */
  memset(sieve->cc1_muls, 0xFF, sizeof(uint32_t *) * 
                                sieve->layers * 
                                sieve->max_prime_index);

  memset(sieve->cc2_muls, 0xFF, sizeof(uint32_t *) * 
                                sieve->layers * 
                                sieve->max_prime_index);


  #ifdef DEBUG
  //print_sieve(sieve);
  #endif
}

/**
 * initializes a given sieve for the first
 */
void init_sieve(Sieve *sieve, Opts *const opts) {

  memset(sieve, 0, sizeof(Sieve));
  sieve_set_header(sieve, opts->header);
    
  mpz_init(sieve->mpz_test_origin);
  mpz_init(sieve->mpz_multiplier);
  mpz_init(sieve->mpz_primorial_primes);
  mpz_init(sieve->mpz_reminder);
  mpz_init(sieve->mpz_hash);
  mpz_init(sieve->mpz_hash_primorial);
  mpz_init(sieve->mpz_tmp);

  mpz_set(sieve->mpz_hash_primorial,   opts->mpz_hash_primorial);
  mpz_set(sieve->mpz_primorial_primes, opts->mpz_primorial_primes);



  sieve->min_prime_index      = opts->n_primes_in_primorial; // TODO bettre name this is the number of the first 
                                                             // prime not used in primorial
  sieve->opts                 = opts;
  sieve->poolshare            = opts->poolshare;
  sieve->primes               = opts->primes;
  sieve->chain_length         = opts->chain_length;
  sieve->extensions           = opts->n_sieve_extensions;
  sieve->layers               = sieve->extensions + sieve->chain_length;
  sieve->max_prime_index      = opts->max_prime_index;
  sieve->cache_bits           = opts->cachebits;
  sieve->sieve_words          = opts->sieve_words;

  /* make shure sieve size is a multiply of 2 * cache_bits */
  /* (the halfe of the sieve size shoul be a muptiply of cache bytes) */
  sieve->size = opts->sievesize;
  sieve->candidate_bytes = sizeof(sieve_t) * sieve->sieve_words;
  
  sieve->cc1   = malloc(sieve->candidate_bytes);
  sieve->cc2   = malloc(sieve->candidate_bytes);
  sieve->twn   = malloc(sieve->candidate_bytes);
  sieve->all   = malloc(sieve->candidate_bytes);

  /* for cc1 and cc2 chains, for each layer */
  sieve->cc1_muls = malloc(sizeof(uint32_t *) * 
                           sieve->layers * 
                           sieve->max_prime_index);

  sieve->cc2_muls = malloc(sizeof(uint32_t *) * 
                           sieve->layers * 
                           sieve->max_prime_index);

  init_test_params(&sieve->test_params);

  sieve->stats.start_time = gettime_usec();
}

/**
 * frees all used resauces of the sieve
 */
void free_sieve(Sieve *sieve) {

  free(sieve->cc1);
  free(sieve->cc2);
  free(sieve->twn);
  free(sieve->all);
  free(sieve->cc1_muls);
  free(sieve->cc2_muls);

  clear_test_params(&sieve->test_params);

  mpz_clear(sieve->mpz_test_origin);
  mpz_clear(sieve->mpz_multiplier);
  mpz_clear(sieve->mpz_primorial_primes);
  mpz_clear(sieve->mpz_reminder);
  mpz_clear(sieve->mpz_hash);
  mpz_clear(sieve->mpz_hash_primorial);
  mpz_clear(sieve->mpz_tmp);
}

/**
 * Extended Euclidean algorithm to calculate the inverse of 
 * a in finite field defined by p
 */
static inline uint32_t invert(const uint32_t a, const uint32_t p) {

  int rem0 = p, rem1 = a % p, rem2;
  int aux0 = 0, aux1 = 1, aux2;
  int quotient, inverse;

  for (;;) {

    if (rem1 <= 1) {
    
      inverse = aux1;
      break;
    }

    rem2     = rem0 % rem1;
    quotient = rem0 / rem1;
    aux2     = -quotient * aux1 + aux0;

    if (rem2 <= 1) {
    
      inverse = aux2;
      break;
    }

    rem0     = rem1 % rem2;
    quotient = rem1 / rem2;
    aux0     = -quotient * aux2 + aux1;

    if (rem0 <= 1) {
    
      inverse = aux0;
      break;
    }

    rem1     = rem2 % rem0;
    quotient = rem2 / rem0;
    aux1     = -quotient * aux0 + aux2;
  }

  return (inverse + p) % p;
}

/**
 * sieves all primes in the given intervall, and layer (cache optimation)
 * for the given candidates array
 */
static inline void sieve_from_to(const Sieve *const sieve, 
                                 sieve_t *const candidates,
                                 uint32_t *const multipliers,
                                 const uint32_t end,
                                 const uint32_t layer) {
  uint32_t i;
  for (i = sieve->min_prime_index; 
       sieve->active && i < sieve->max_prime_index; 
       i++) {

    /* current prime */
    const uint32_t prime = sieve->primes->ptr[i];
    
    /* current factor */
    uint32_t factor = multipliers[i * sieve->layers + layer];

    sieve_t word = bit_word(factor);
    const uint32_t rotate = prime % word_bits;

    /* progress the given range of the sieve */
    for (; factor < end; factor += prime) {
      word_at(candidates, factor) |= word;
      word = (word << rotate) | (word >> (word_bits - rotate));
    }

    /* save the factor for the next round */
    multipliers[i * sieve->layers + layer] = factor;
  }
}

/**
 * test the found candidates 
 */
static inline void test_candidates(Sieve *const sieve, 
                                   const mpz_t mpz_primorial,
                                   const uint32_t extension) {

  sieve_t *const all      = sieve->all;
  sieve_t *const twn      = sieve->twn;
  sieve_t *const cc1      = sieve->cc1;
  SieveStats *const stats = &sieve->stats;

  uint32_t i;
  for (i = (extension ? sieve->sieve_words / 2 : 0); 
       sieve->active && i < sieve->sieve_words; 
       i++) {
    
    const sieve_t word = all[i];
    
    /* skipp a word if no candidates in it */
    if (word == word_max) continue; 

    sieve_t n, bit;
    for (n = 1, bit = 1; n != 0; n <<= 1, bit++) {

      /* fond an not sieved index */
      if ((word & n) == 0) {

        stats->tests++;

        /* break if sieve shoud terminate */
        if (!sieve->active) break;
        
        /* origins = primorial * index << extension */
        mpz_mul_ui(sieve->mpz_test_origin, 
                   mpz_primorial, 
                   (word_bits * i + bit) << extension);

        uint32_t difficulty;
        char     *type;

        /* bi-twin candidate */
        if ((twn[i] & n) == 0) {
          
          type = BI_TWIN_CHAIN;
          difficulty = 1<<24;; //twn_chain_test(sieve->mpz_test_origin,
                          //            &sieve->test_params); 

          stats->twn[chain_length(difficulty)]++;
        /* cc1 candidate */
        } else if ((cc1[i] & n) == 0) {

          type = FIRST_CUNNINGHAM_CHAIN;
          difficulty = 1<<24;; //cc1_chain_test(sieve->mpz_test_origin,
                          //            &sieve->test_params);

          stats->cc1[chain_length(difficulty)]++;
        /* cc2 candidate */
        } else {
        
          type = SECOND_CUNNINGHAM_CHAIN;
          difficulty = 1<<24; //cc2_chain_test(sieve->mpz_test_origin,
                          //            &sieve->test_params);

          stats->cc2[chain_length(difficulty)]++;
        }

        if (chain_length(difficulty) >= sieve->poolshare) {
          
          mpz_mul_ui(sieve->mpz_multiplier, // TODO hier error
                    sieve->mpz_primorial_primes, 
                    (word_bits * i) << extension);

          mpz_to_ary(sieve->header.primemultiplier, sieve->mpz_multiplier);
          submit_share(sieve->opts, &sieve->header, type, difficulty); 
        }
      }
    }
  }
} 


/**
 * run the sieve
 * primorial is x * 2 * 3 * 5 * 11 * 17 * ...
 *
 * where x is hash / (2 * 3 * 5 * 7)
 *
 * the bitverktor of the sieves H, 2H, 3H, 4H, ... ,nH
 * where H is the primorial
 */
void sieve_run(Sieve *const sieve, const mpz_t mpz_primorial) {

  uint32_t *const cc1_muls      = sieve->cc1_muls;
  uint32_t *const cc2_muls      = sieve->cc2_muls;
  sieve_t *const twn            = sieve->twn;
  sieve_t *const cc2            = sieve->cc2;
  sieve_t *const cc1            = sieve->cc1;
  sieve_t *const all            = sieve->all;
  const uint32_t *const primes  = sieve->primes->ptr;

  /* generate the multiplicators for the first layer first */
  uint32_t i;
  for (i = 0;//sieve->min_prime_index; 
       sieve->active && i < sieve->max_prime_index; 
       i++) {

    /* current prime */
    const uint32_t prime = primes[i];

    /* modulo = primorial % prime */
    const uint32_t modulo = (uint32_t) mpz_tdiv_ui(mpz_primorial, prime);

    /* nothing in the sieve is divisible by this prime */
    if (modulo == 0) continue;
    

    /**
     * al indices of the sieve lets call them 1H, 2H, 3H, ... nH
     * ar prime origins, so we whant to know when kH +/- 1, is dividible
     * by prime.
     * This is so when kH % p == 1 or kH %p == p - 1,
     *
     * now we are using the extendet euclid to calulate the invert of
     * kH % p, so whe have a factor which fitts to the k use above
     * k = factor + prime * x
     */
    uint32_t factor = invert(modulo, prime);

    /**
     * inverse of two to faster calculate the next number in chain to sieve 
     * TODO expailn better
     */
    const uint32_t two_inverse = invert(2, prime);

    const uint32_t index = sieve->layers * i;
    uint32_t l;

    for (l = 0; l < sieve->layers; l++) {

      cc1_muls[index + l] = factor;
      cc2_muls[index + l] = prime - factor;

      /* calc factor for the next number in chain */
      factor = (factor * two_inverse) % prime;
    }
  }

  /* calculate the wi-twin cc1 and cc2 layers */
  const uint32_t twn_cc1_layers = (sieve->chain_length + 1) / 2 - 1;
  const uint32_t twn_cc2_layers = sieve->chain_length       / 2 - 1;
  uint32_t l;

  /* do the actual sieveing */
  for (l = 0; sieve->active && l < sieve->chain_length; l++) {

    for (i = sieve->cache_bits; 
         sieve->active && i <= sieve->size; 
         i += sieve->cache_bits) {

      sieve_from_to(sieve, cc1, cc1_muls, i, l);
    }

    /* cc2 */
    for (i = sieve->cache_bits; 
         sieve->active && i <= sieve->size; 
         i += sieve->cache_bits) {

      sieve_from_to(sieve, cc2, cc2_muls, i, l);
    }

    /* copy cc2 layers to twn chains */
    if (twn_cc2_layers == l) 
      memcpy(twn, cc2, sieve->candidate_bytes);

    /* applay cc1 layers to twn chains */
    if (twn_cc1_layers == l) {
      for (i = 0; i < sieve->sieve_words; i++)
        twn[i] |= cc1[i];
    }
  }

  /* create the final set of candidates */
  for (i = 0; i < sieve->sieve_words; i++)
    all[i] = cc1[i] & cc2[i] & twn[i];

  /* run the feramt test on the remaining candidates */
  test_candidates(sieve, mpz_primorial, 0);
  return 0;

  uint32_t e;
  /* do the actual sieveing for the extensions */
  for (e = 1; sieve->active && e <= sieve->extensions; e++) {

    /* only second half of the arrays are use */
    memset(cc1 + sieve->sieve_words / 2, 0, sieve->candidate_bytes / 2);
    memset(cc2 + sieve->sieve_words / 2, 0, sieve->candidate_bytes / 2);
    memset(twn + sieve->sieve_words / 2, 0, sieve->candidate_bytes / 2);


    for (l = e; sieve->active && l < (sieve->chain_length + e); l++) {
 
      /* cc1 */
      for (i = sieve->size / 2 + sieve->cache_bits; 
           sieve->active && i <= sieve->size; 
           i += sieve->cache_bits) {
 
        sieve_from_to(sieve, cc1, cc1_muls, i, l);
      }
 
      /* cc2 */
      for (i = sieve->size / 2 + sieve->cache_bits; 
           sieve->active && i <= sieve->size; 
           i += sieve->cache_bits) {
 
        sieve_from_to(sieve, cc2, cc2_muls, i, l);
      }
 
      /* copy cc2 layers to twn chains */
      if (twn_cc2_layers == l + e) 
        memcpy(twn, cc2 + sieve->sieve_words / 2, sieve->candidate_bytes / 2);
 
      /* applay cc1 layers to twn chains */
      if (twn_cc1_layers == l + e) {
        for (i = sieve->sieve_words / 2; i < sieve->sieve_words; i++)
          twn[i] |= cc1[i];
      }
    }

    /* create the all set of candidates */
    for (i = 0; i < sieve->sieve_words; i++)
      all[i] = cc1[i] & cc2[i] & twn[i];
    
    
    /* run the feramt test on the remaining candidates */
    test_candidates(sieve, mpz_primorial, e);
 }
}
