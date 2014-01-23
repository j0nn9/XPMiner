/**
 * implementation of the Sieve of Eratosthenes for finding prime chains
 */
#include <sys/time.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
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
#define word_at(ary, i) (ary)[(i) % word_len]

/**
 * returns a word with the given bit index settet
 */
#define bit_word(i) (1 << ((i) % word_len))

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
 * reinitilaizes an given sieve
 */
void reinit_sieve(Sieve *sieve, const Opts *opts) {

  sieve->active = 1;

  memset(sieve->cc1,   0, sieve->candidate_bytes);
  memset(sieve->cc2,   0, sieve->candidate_bytes);
  memset(sieve->twn,   0, sieve->candidate_bytes);
  memset(sieve->final, 0, sieve->candidate_bytes);

  mpz_set(sieve->mpz_primorial_primes, opts->mpz_primorial_primes);

  memcpy(&sieve->header, opts->header, sizeof(BlockHeader));

  sieve->min_prime_index  = opts->n_primes_in_primorial + 1;

  /* for cc1 and cc2 chains, for each layer */
  memset(sieve->cc1_muls, 0xFF, sizeof(uint32_t *) * 
                                sieve->layers * 
                                sieve->max_prime_index);

  memset(sieve->cc2_muls, 0xFF, sizeof(uint32_t *) * 
                                sieve->layers * 
                                sieve->max_prime_index);
}

/**
 * initializes a given sieve for the first
 */
void init_sieve(Sieve *sieve, Opts *const opts) {

  memset(sieve, 0, sizeof(Sieve));
    
  mpz_init(sieve->mpz_test_origin);
  mpz_init(sieve->mpz_multiplier);
  mpz_init(sieve->mpz_primorial);
  mpz_init(sieve->mpz_primorial_primes);
  mpz_init(sieve->mpz_reminder);
  mpz_init(sieve->mpz_hash);
  mpz_init(sieve->mpz_hash_primorial);

  mpz_set(sieve->mpz_hash_primorial, opts->mpz_hash_primorial);

  sieve->opts                 = opts;
  sieve->primes               = opts->primes;
  sieve->chain_length         = opts->chain_length;
  sieve->extensions           = opts->n_sieve_extensions;
  sieve->layers               = sieve->extensions + sieve->chain_length;
  sieve->max_prime_index      = opts->max_prime_index;
  sieve->cache_bytes          = (opts->cachebytes / sizeof(sieve_t)) * 
                                sizeof(sieve_t);

  /* make shure sieve size is a multiply of 2 * cache_bytes */
  /* (the halfe of the sieve size shoul be a muptiply of cache bytes) */
  sieve->size             = (opts->sievesize / (2 * sieve->cache_bytes)) *
                            (2 * sieve->cache_bytes);

  sieve->candidate_bytes = sizeof(sieve_t) * sieve->size;
  
  sieve->cc1   = malloc(sieve->candidate_bytes);
  sieve->cc2   = malloc(sieve->candidate_bytes);
  sieve->twn   = malloc(sieve->candidate_bytes);
  sieve->final = malloc(sieve->candidate_bytes);

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
                                 const uint32_t start, // TODO remove after debuging
                                 const uint32_t end,
                                 const uint32_t layer) {

  uint32_t i;
  for (i = sieve->min_prime_index; 
       sieve->active && i < sieve->max_prime_index; 
       i++) {

    /* current prime */
    const uint32_t prime = sieve->primes->ptr[i];
    
    /* current factor */
    const uint32_t factor = sieve->cc1_muls[i * sieve->layers + layer];

    sieve_t word = bit_word(factor);

    #ifdef DEBUG
    if (factor < start)
      printf("[EE] failed factor < start!\n");
    #endif

    const uint32_t rotate = prime % word_len;

    /* progress the given range of the sieve */
    uint32_t n;
    for (n = factor; n < end; n += prime) {
      
      word_at(candidates, n) |= word;
      word = (word << rotate) | (word >> (word_len - rotate));
    }

    /* save the factor for the next round */
    sieve->cc1_muls[i * sieve->layers + layer] = n;
  }
}

/**
 * test the found candidates 
 */
static inline void test_candidates(Sieve *const sieve, 
                                   const uint32_t extension) {

  sieve_t *const final    = sieve->final;
  sieve_t *const twn      = sieve->twn;
  sieve_t *const cc1      = sieve->cc1;
  SieveStats *const stats = &sieve->stats;

  uint32_t i;
  for (i = 0; sieve->active && i < sieve->size; i++) {
    
    const sieve_t word = final[i];
    
    /* skipp a word if no candidates in it */
    while (word == word_max) continue;

    sieve_t n;
    for (n = 1; n != 0; n <<= 1) {

      /* fond an not sieved index */
      if ((word & n) == 0) {

        stats->tests++;

        /* break if sieve shoud terminate */
        if (!sieve->active) break;
        
        /* origins = primorial * index << extension */
        mpz_mul_ui(sieve->mpz_test_origin, 
                   sieve->mpz_primorial, 
                   (n * i) << extension);

        uint32_t difficulty;
        char     *type;

        /* bi-twin candidate */
        if ((twn[i] & n) == 0) {
          
          type = BI_TWIN_CHAIN;
          difficulty = twn_chain_test(sieve->mpz_test_origin,
                                      &sieve->test_params);

          stats->twn[chain_length(difficulty)]++;
        /* cc1 candidate */
        } else if ((cc1[i] & n) == 0) {

          type = FIRST_CUNNINGHAM_CHAIN;
          difficulty = cc1_chain_test(sieve->mpz_test_origin,
                                      &sieve->test_params);

          stats->cc1[chain_length(difficulty)]++;
        /* cc2 candidate */
        } else {
        
          type = SECOND_CUNNINGHAM_CHAIN;
          difficulty = cc2_chain_test(sieve->mpz_test_origin,
                                      &sieve->test_params);

          stats->cc2[chain_length(difficulty)]++;
        }

        if (chain_length(difficulty) > sieve->chain_length) {
          
          mpz_mul_ui(sieve->mpz_multiplier, 
                    sieve->mpz_primorial_primes, 
                    (n * i) << extension);

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

  uint32_t *const cc1_muls = sieve->cc1_muls;
  uint32_t *const cc2_muls = sieve->cc2_muls;
  sieve_t *const twn      = sieve->twn;
  sieve_t *const cc2      = sieve->cc2;
  sieve_t *const cc1      = sieve->cc1;
  sieve_t *const final    = sieve->final;
  const uint32_t *const primes  = sieve->primes->ptr;

  /* generate the multiplicators for the first layer first */
  uint32_t i;
  for (i = sieve->min_prime_index; 
       sieve->active && i < sieve->max_prime_index; 
       i++) {

    /* current prime */
    const uint32_t prime = primes[i];

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
    uint32_t factor = invert(mpz_mod_ui(NULL, mpz_primorial, prime), prime);

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
  for (l = 0; sieve->active && l < sieve->chain_length; i++) {

    /* cc1 */
    for (i = sieve->cache_bytes; 
         sieve->active && i < sieve->size; 
         i += sieve->cache_bytes) {

      sieve_from_to(sieve, cc1, i - sieve->cache_bytes, i, l);
    }

    /* cc2 */
    for (i = sieve->cache_bytes; 
         sieve->active && i < sieve->size; 
         i += sieve->cache_bytes) {

      sieve_from_to(sieve, cc2, i - sieve->cache_bytes, i, l);
    }

    /* copy cc2 layers to twn chains */
    if (twn_cc2_layers == l) 
      memcpy(twn, cc2, sieve->candidate_bytes);

    /* applay cc1 layers to twn chains */
    if (twn_cc1_layers == l) {
      for (i = 0; i < sieve->size; i++)
        twn[i] |= cc1[i];
    }
  }

  /* create the final set of candidates */
  for (i = 0; i < sieve->size; i++)
    final[i] = cc1[i] & cc2[i] && twn[i];

  /* run the feramt test on the remaining candidates */
  test_candidates(sieve, 0);
  

  uint32_t e;
  /* do the actual sieveing for the extensions */
  for (e = 1; sieve->active && e < sieve->extensions; e++) {

    /* only second half of the arrays are use */
    memset(cc1 + sieve->size / 2, 0, sieve->candidate_bytes / 2);
    memset(cc2 + sieve->size / 2, 0, sieve->candidate_bytes / 2);
    memset(twn + sieve->size / 2, 0, sieve->candidate_bytes / 2);


    for (l = e; sieve->active && l < sieve->chain_length + e; i++) {
 
      /* cc1 */
      for (i = sieve->size / 2; 
           sieve->active && i < sieve->size; 
           i += sieve->cache_bytes) {
 
        sieve_from_to(sieve, cc1, i - sieve->cache_bytes, i, l);
      }
 
      /* cc2 */
      for (i = sieve->size / 2; 
           sieve->active && i < sieve->size; 
           i += sieve->cache_bytes) {
 
        sieve_from_to(sieve, cc2, i - sieve->cache_bytes, i, l);
      }
 
      /* copy cc2 layers to twn chains */
      if (twn_cc2_layers == l) 
        memcpy(twn, cc2 + sieve->size / 2, sieve->candidate_bytes / 2);
 
      /* applay cc1 layers to twn chains */
      if (twn_cc1_layers == l) {
        for (i = sieve->size / 2; i < sieve->size; i++)
          twn[i] |= cc1[i];
      }
    }

    /* create the final set of candidates */
    for (i = 0; i < sieve->size; i++)
      final[i] = cc1[i] & cc2[i] && twn[i];
    
    
    /* run the feramt test on the remaining candidates */
    test_candidates(sieve, e);
 }
}
