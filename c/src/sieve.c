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

#define min(x, y) (((x) < (y)) ? (x) : (y))
#define max(x, y) (((x) > (y)) ? (x) : (y))

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

  memset(sieve->cc1, 0, sieve->candidate_bytes);
  memset(sieve->cc2, 0, sieve->candidate_bytes);

  memset(sieve->ext_cc1, 0, sieve->candidate_bytes * sieve->extensions);
  memset(sieve->ext_cc2, 0, sieve->candidate_bytes * sieve->extensions);
  memset(sieve->ext_twn, 0, sieve->candidate_bytes * sieve->extensions);

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
  
  sieve->cc1 = malloc(sieve->candidate_bytes);
  sieve->cc2 = malloc(sieve->candidate_bytes);
  sieve->twn = malloc(sieve->candidate_bytes);
  sieve->all = malloc(sieve->candidate_bytes);
  sieve->tmp = malloc(sieve->candidate_bytes);

  sieve->ext_cc1 = malloc(sieve->candidate_bytes * sieve->extensions);
  sieve->ext_cc2 = malloc(sieve->candidate_bytes * sieve->extensions);
  sieve->ext_twn = malloc(sieve->candidate_bytes * sieve->extensions);

  sieve->cc1_layer = malloc(sieve->candidate_bytes);
  sieve->cc2_layer = malloc(sieve->candidate_bytes);

  /* for cc1 and cc2 chains, for each layer */
  sieve->cc1_muls = malloc(sizeof(uint32_t *) * 
                           sieve->layers * 
                           sieve->max_prime_index);

  sieve->cc2_muls = malloc(sizeof(uint32_t *) * 
                           sieve->layers * 
                           sieve->max_prime_index);

  init_test_params(&sieve->test_params);

  sieve->stats.start_time = gettime_usec();

  /**
   * create the inverses of two for all used primes
   */
  uint32_t i;
  sieve->two_inverses = malloc(sizeof(uint32_t) * sieve->primes->len);
  for (i = 0; i < sieve->primes->len; i++) {
    
    sieve->two_inverses[i] = invert(2, sieve->primes->ptr[i]);
    
    if (sieve->int64_arithmetic == 0 &&
        UINT32_MAX / sieve->two_inverses[i] < sieve->primes->ptr[i]) {

      sieve->int64_arithmetic = i;
    }
  }
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
 * sieves all primes in the given intervall, and layer (cache optimation)
 * for the given candidates array
 */
static inline void sieve_from_to(const Sieve *const sieve, 
                                 sieve_t *const candidates,
                                 uint32_t *const multipliers,
                                 const uint32_t start,
                                 const uint32_t end,
                                 const uint32_t layer) {
  /* wipe the array */
  if (start >= end) return; // TODO remove
  memset(candidates + (start / word_bits), 0, ((end - start) / 8));

  uint32_t i;
  for (i = sieve->min_prime_index; 
       sieve->active && i < sieve->max_prime_index; 
       i++) {

    /* current prime */
    const uint32_t prime = sieve->primes->ptr[i];
    
    /* current factor */
    uint32_t factor = multipliers[i * sieve->layers + layer];

    /* adjust factor */
    if (factor < start)
      factor += (start - factor + prime - 1) / prime * prime;

    sieve_t word = bit_word(factor);
    const uint32_t rotate = prime % word_bits;

    /* progress the given range of the sieve */
    for (; factor < end; factor += prime) {
      word_at(candidates, factor) |= word;
      word = (word << rotate) | (word >> (word_bits - rotate));
    }

    /* save the factor for the next round */
    //multipliers[i * sieve->layers + layer] = factor;
  }
}


/**
 * test the found candidates 
 */
static inline void test_candidates(Sieve *const sieve, 
                                   sieve_t *const cc1,
                                   sieve_t *const twn,
                                   const mpz_t mpz_primorial,
                                   const uint32_t extension) {

  sieve_t *const all      = sieve->all;
  SieveStats *const stats = &sieve->stats;

  uint32_t i;
  for (i = (extension ? (sieve->sieve_words / 2) : 0); 
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

  uint32_t *const cc1_muls       = sieve->cc1_muls;
  uint32_t *const cc2_muls       = sieve->cc2_muls;
  sieve_t *const twn             = sieve->twn;
  sieve_t *const cc2             = sieve->cc2;
  sieve_t *const cc1             = sieve->cc1;
  sieve_t *const all             = sieve->all;
  sieve_t *const tmp             = sieve->tmp;
  sieve_t *const cc1_layer       = sieve->tmp;
  sieve_t *const cc2_layer       = sieve->tmp;
  sieve_t *const ext_twn         = sieve->ext_twn;
  sieve_t *const ext_cc2         = sieve->ext_cc2;
  sieve_t *const ext_cc1         = sieve->ext_cc1;
  const uint32_t *const primes   = sieve->primes->ptr;
  const uint32_t *const inverses = sieve->two_inverses;

  /* generate the multiplicators for the first layer first */
  uint32_t i;
  for (i = sieve->min_prime_index; 
       sieve->active && i < sieve->max_prime_index; 
       i++) {

    /* current prime */
    const uint32_t prime = primes[i];

    /* modulo = primorial % prime */
    const uint32_t modulo = (uint32_t) mpz_tdiv_ui(mpz_primorial, prime);

    /* nothing in the sieve is divisible by this prime */
    if (modulo == 0) continue;

    uint32_t factor = invert(modulo, prime);
    const uint32_t two_inverse = inverses[i];

    
    const uint32_t offset = sieve->layers * i;
    uint32_t l;

    if (i < sieve->int64_arithmetic) {

      for (l = 0; l < sieve->layers; l++) {
     
        cc1_muls[offset + l] = factor;
        cc2_muls[offset + l] = prime - factor;
     
        /* calc factor for the next number in chain */
        factor = (factor * two_inverse) % prime;
      }
    } else {

      for (l = 0; l < sieve->layers; l++) {
     
        cc1_muls[offset + l] = factor;
        cc2_muls[offset + l] = prime - factor;
     
        /* calc factor for the next number in chain */
        factor = (uint32_t) (((uint64_t) factor) * 
                             ((uint64_t) two_inverse)) % ((uint64_t) prime);
      }
    }
  }


#if 0
  // Process the array in chunks that fit the L1 cache
  const unsigned int sieve_rounds = (sieve->size + sieve->cache_bits - 1) / sieve->cache_bits;

  // Calculate the number of CC1 and CC2 layers needed for BiTwin candidates
  const unsigned int twn_cc1_layers = (sieve->chain_length + 1) / 2;
  const unsigned int twn_cc2_layers = sieve->chain_length / 2;

  // Only 50% of the array is used in extensions
  const unsigned int ext_min_multi = sieve->size / 2;
  const unsigned int ext_min_word = ext_min_multi / word_bits;

  // Loop over each array one at a time for optimal L1 cache performance
  unsigned int j;
  for (j = 0; j < sieve_rounds; j++)
  {
    const unsigned int min_multi = sieve->cache_bits * j;
    const unsigned int max_multi = min(sieve->cache_bits * (j + 1), sieve->size);
    const unsigned int used_ext_min_multi = max(min_multi, ext_min_multi);
    const unsigned int min_word = min_multi / word_bits;
    const unsigned int max_word = (max_multi + word_bits - 1) / word_bits;
    const unsigned int used_ext_min_word = max(min_word, ext_min_word);
    if (!sieve->active)
      break;  // new block

    // Loop over the layers
    unsigned int l;
    for (l = 0; l < sieve->layers; l++) 
    {
      if (!sieve->active)
        break;  // new block
      if (l < sieve->chain_length)
      {
        sieve_from_to(sieve, cc1_layer, cc1_muls, min_multi, max_multi, l);
        sieve_from_to(sieve, cc2_layer, cc2_muls, min_multi, max_multi, l);
      }
      else
      {
        // Optimize: First halves of the arrays are not needed in the extensions
        sieve_from_to(sieve, cc1_layer, cc1_muls, used_ext_min_multi, max_multi, l);
        sieve_from_to(sieve, cc2_layer, cc2_muls, used_ext_min_multi, max_multi, l);
      }

      // Apply the layer to the primary sieve arrays
      if (l < sieve->chain_length)
      {
        if (l < twn_cc2_layers)
        {
          unsigned int w;
          for (w = min_word; w < max_word; w++)
          {
            cc1[w] |= cc1_layer[w];
            cc2[w] |= cc2_layer[w];
            twn[w] |= cc1_layer[w] | cc2_layer[w];
          }
        }
        else if (l < twn_cc1_layers)
        {
          unsigned int w;
          for (w = min_word; w < max_word; w++)
          {
            cc1[w] |= cc1_layer[w];
            cc2[w] |= cc2_layer[w];
            twn[w] |= cc1_layer[w];
          }
        }
        else
        {
          unsigned int w;
          for (w = min_word; w < max_word; w++)
          {
            cc1[w] |= cc1_layer[w];
            cc2[w] |= cc2_layer[w];
          }
        }
      }

      // Apply the layer to extensions
      unsigned int e;
      for (e = 0; e < sieve->extensions; e++)
      {
        const unsigned int layer_offset = e + 1;
        if (l >= layer_offset && l < sieve->chain_length + layer_offset)
        {
          const unsigned int ext_layer = l - layer_offset;
          sieve_t *p_ext_cc1 = ext_cc1 + e * sieve->sieve_words;
          sieve_t *p_ext_cc2 = ext_cc2 + e * sieve->sieve_words;
          sieve_t *p_ext_twn = ext_twn + e * sieve->sieve_words;
          if (ext_layer < twn_cc2_layers)
          {
            unsigned int w;
            for (w = used_ext_min_word; w < max_word; w++)
            {
              p_ext_cc1[w] |= cc1_layer[w];
              p_ext_cc2[w] |= cc2_layer[w];
              p_ext_twn[w] |= cc1_layer[w] | cc2_layer[w];
            }
          }
          else if (ext_layer < twn_cc1_layers)
          {
            unsigned int w;
            for (w = used_ext_min_word; w < max_word; w++)
            {
              p_ext_cc1[w] |= cc1_layer[w];
              p_ext_cc2[w] |= cc2_layer[w];
              p_ext_twn[w] |= cc1_layer[w];
            }
          }
          else
          {
            unsigned int w;
            for (w = used_ext_min_word; w < max_word; w++)
            {
              p_ext_cc1[w] |= cc1_layer[w];
              p_ext_cc2[w] |= cc2_layer[w];
            }
          }
        }
      }
    }
  }
  

  (void) tmp;
  uint32_t e;
#else
  /* calculate the wi-twin cc1 and cc2 layers */
  const uint32_t twn_cc1_layers = (sieve->chain_length + 1) / 2 - 1;
  const uint32_t twn_cc2_layers = sieve->chain_length       / 2 - 1;
  uint32_t l, w, e;

  /* sieve the cc2 candidates */
  for (i = 0; sieve->active && i < sieve->size / 2; i += sieve->cache_bits) {
    for (l = 0; sieve->active && l < sieve->layers; l++) {

      sieve_from_to(sieve, tmp, cc2_muls, i, i + sieve->cache_bits, l);  // TODO organize muls cace optimized


      if (l < sieve->chain_length) {

        for (w = i / word_bits;
             w < ((i + sieve->cache_bits) / word_bits); 
             w++) {

          cc2[w] |= tmp[w];
        }
      }

      /* copy layers to the twn candidates */
      if (l == twn_cc2_layers) {

        uint32_t offset = i / word_bits;
        memcpy(twn + offset, cc2 + offset, sieve->cache_bits / 8);
      }
    }
  }

  /* sieve the cc1 candidates */
  for (i = 0; sieve->active && i < sieve->size / 2; i += sieve->cache_bits) {
    for (l = 0; sieve->active && l < sieve->layers; l++) {

      sieve_from_to(sieve, tmp, cc1_muls, i, i + sieve->cache_bits, l);  // TODO organize muls cace optimized


      if (l < sieve->chain_length) {

        for (w = i / word_bits; 
             w < ((i + sieve->cache_bits) / word_bits); 
             w++) {

          cc1[w] |= tmp[w];
        }
      }

      /* applay layers to the twn candidates */
      if (l == twn_cc1_layers) {

        for (w = i / word_bits; 
             w < ((i + sieve->cache_bits) / word_bits); 
             w++) {

          twn[w] |= cc1[w];
        }
      }
    }
  }

  /* sieve the cc2 candidates */
  for (i = sieve->size / 2; sieve->active && i < sieve->size; i += sieve->cache_bits) {
    for (l = 0; sieve->active && l < sieve->layers; l++) {

      sieve_from_to(sieve, tmp, cc2_muls, i, i + sieve->cache_bits, l);  // TODO organize muls cace optimized


      if (l < sieve->chain_length) {

        for (w = i / word_bits;
             w < ((i + sieve->cache_bits) / word_bits); 
             w++) {

          cc2[w] |= tmp[w];
        }
      }

      /* copy layers to the twn candidates */
      if (l == twn_cc2_layers) {

        uint32_t offset = i / word_bits;
        memcpy(twn + offset, cc2 + offset, sieve->cache_bits / 8);
      }


      /* applay layers to the extensions */
      for (e = 0; sieve->active && e < sieve->extensions; e++) {
     
        if (e < l && l <= e + sieve->chain_length) {
     
          sieve_t *ptr = ext_cc2 + e * sieve->sieve_words;
     
          for (w = i / word_bits; 
               w < ((i + sieve->cache_bits) / word_bits); 
               w++) {

            ptr[w] |= tmp[w];
          }
        }
#if 1
        /* copy layers to the extended twn candidates */
        if ((l - (e + 1)) == twn_cc2_layers) {
          uint32_t offset = i / word_bits + e * sieve->sieve_words;
          memcpy(ext_twn + offset, ext_cc2 + offset, sieve->cache_bits / 8);
        }
#else

        if (e < l && l <= e + sieve->chain_length && (l - (e + 1)) <= twn_cc2_layers) {
     
          sieve_t *ptr = ext_twn + e * sieve->sieve_words;
     
          for (w = i / word_bits; 
               w < ((i + sieve->cache_bits) / word_bits); 
               w++) {

            ptr[w] |= tmp[w];
          }
        }
#endif
      }
    }
  }

  /* sieve the cc1 candidates */
  for (i = sieve->size / 2; sieve->active && i < sieve->size; i += sieve->cache_bits) {
    for (l = 0; sieve->active && l < sieve->layers; l++) {

      sieve_from_to(sieve, tmp, cc1_muls, i, i + sieve->cache_bits, l);  // TODO organize muls cace optimized


      if (l < sieve->chain_length) {

        for (w = i / word_bits; 
             w < ((i + sieve->cache_bits) / word_bits); 
             w++) {

          cc1[w] |= tmp[w];
        }
      }

      /* applay layers to the twn candidates */
      if (l == twn_cc1_layers) {

        for (w = i / word_bits; 
             w < ((i + sieve->cache_bits) / word_bits); 
             w++) {

          twn[w] |= cc1[w];
        }
      }


      /* applay layers to the extensions */
      for (e = 0; sieve->active && e < sieve->extensions; e++) {
     
        if (e < l && l <= e + sieve->chain_length) {
     
          sieve_t *ptr = ext_cc1 + e * sieve->sieve_words;
     
          for (w = i / word_bits; 
               w < ((i + sieve->cache_bits) / word_bits); 
               w++) {

            ptr[w] |= tmp[w];
          }
        }
#if 1 
        /* copy layers to the extended twn candidates */  // TODO es scheint das die anzahl an treffern richtig ist wenn ich hier das +1 endferne weiter forschen pb das stimmt und wen ja wieso und ob das dann auch mit den treffern an richtigen chains übereinstimmt
        if ((l - (e + 1)) == twn_cc1_layers) {
          sieve_t *ptr_cc1 = ext_cc1 + e * sieve->sieve_words;
          sieve_t *ptr_twn = ext_twn + e * sieve->sieve_words;
     
          for (w = i / word_bits; 
               w < ((i + sieve->cache_bits) / word_bits); 
               w++) {

            ptr_twn[w] |= ptr_cc1[w];
          }
        }
#else

        if (e < l && l <= e + sieve->chain_length && (l - (e + 1)) <= twn_cc1_layers) {
     
          sieve_t *ptr = ext_twn + e * sieve->sieve_words;
     
          for (w = i / word_bits; 
               w < ((i + sieve->cache_bits) / word_bits); 
               w++) {

            ptr[w] |= tmp[w];
          }
        }
#endif
      }
    }
  }
#endif


  /* create the final set of candidates */
  for (i = 0; i < sieve->sieve_words; i++)
    all[i] = cc1[i] & cc2[i] & twn[i];

  /* run the feramt test on the remaining candidates */
  test_candidates(sieve, cc1, twn, mpz_primorial, 0);

  /* test extended candidates */
  for (e = 0; sieve->active && e < sieve->extensions; e++) {
    
    sieve_t *ptr_cc1 = ext_cc1 + e * sieve->sieve_words;
    sieve_t *ptr_cc2 = ext_cc2 + e * sieve->sieve_words;
    sieve_t *ptr_twn = ext_twn + e * sieve->sieve_words;

    /* create the final set of candidates */
    for (i = sieve->sieve_words / 2; i < sieve->sieve_words; i++) 
      all[i] = ptr_cc1[i] & ptr_cc2[i] & ptr_twn[i];
#if 0
    uint32_t x = 0,y= 0,z=0;
    for (i = sieve->sieve_words / 2; i < sieve->sieve_words; i++) {
      
      sieve_t n;
      for (n = 1; n != 0; n <<= 1) {
        
        if ((ptr_cc1[i] & n) == 0)
          x++;

        if ((ptr_cc2[i] & n) == 0)
          y++;

        if ((ptr_twn[i] & n) == 0)
          z++;
      }

    }
    printf("xcc1 %u, xcc2 %u, xtwn %u\n", x,y,z);
#endif
    /* run the feramt test on the remaining candidates */
    test_candidates(sieve, ptr_cc1, ptr_twn, mpz_primorial, e + 1);
  }
}
