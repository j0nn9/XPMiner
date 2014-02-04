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
 * access the word in which the given bit-index is located
 */
#define word_at(ary, i) (ary)[word_index(i)]

/**
 * returns a word with the given bit index settet
 */
#define bit_word(i) (((sieve_t) 1) << bit_index(i))

/**
 * returns the currnt time in microseconds
 */
static inline uint64_t gettime_usec() {

  struct timeval time;
  if (gettimeofday(&time, NULL) == -1)
    return -1L;

  return time.tv_sec * 1000000L + time.tv_usec;
}

/* the chain length we are sieveing for */
static uint32_t chain_length; 

/* the minimum chain length accepted to submit */
static uint32_t poolshare;

/* the byte length of the candidate bit vektor */
static uint32_t candidate_bytes;

/* the prime table for sieveing */
static const uint32_t *primes;

/**
 * the lowes index to start sieveing the primes 
 *
 * (this can be n_primes_in_primorial
 *  because nothing in the sieve is divisible by any
 *  prime used in the primorial)
 */
static uint32_t min_prime_index;

/* the higes index in the prime table to sieve */
static uint32_t max_prime_index;


/**
 * The number of sieve layers.
 *
 * each sieve layer sieves for one chain candidate
 * for example:
 *  layer 1 sieves all numbers which are not primes
 *  layer n sieves all numbers which are not a chain of length n
 */
static uint32_t layers;
  
/**
 * The number of sieve extension
 *
 * each sieve extension has twice the size of the previous
 * for example
 *
 * the default sieve consists of 0H, 1H, 2H, 3H, ..., nH
 * multiplier, where H is the primorial
 *
 * extension 1 contains out of 0H, 2H, 4H, 6H, ..., 2nH
 * extension 2 contains out of 0H, 4H, 8H, 12H, ..., 4nH
 *
 * and so on
 *
 * the higher extensions containing less primes and the numbers
 * take longer to test, but, the extension can be sieved very efficient,
 * because, the layers overlap:
 *
 * normal sieve consists out of layer 1 to i, where i is chain_length
 *
 * extension 1 consists of 2 to i + 1
 * extension 2 consists of 3 to i + 2
 * extension k consists of (k + 1) to i + k
 */
static uint32_t extensions;

/**
 * prime index after that we have to use 64 bit arithmetic
 */
static uint32_t int64_arithmetic;

/**
 * array of two inverses for the primes 
 */
static uint32_t *two_inverses;

/* the sieve length in bits */
static uint32_t sieve_size;

/* the number of bits to load in cache */
static uint32_t cache_bits;

/* the sieve length in sieve wors */
static uint32_t sieve_words;

/* half the number of bits in the sieve */
static uint32_t bit_half;

/* halve the numer of words in the sieve */
static uint32_t word_half;

/* the number of words cached */
static uint32_t cache_words;

/* the number of bytes cached */
static uint32_t cache_bytes;

/**
 * the additional prim multiplyers used in the primorial 
 * (hash = primorial / fixed_has_multiplier)
 */
static mpz_t mpz_fixed_hash_multiplier;


/**
 * initializes the sieve global variables
 */
void init_sieve_globals() {

  mpz_init(mpz_fixed_hash_multiplier);
  mpz_set(mpz_fixed_hash_multiplier, opts.mpz_fixed_hash_multiplier);

  int64_arithmetic     = opts.int64_arithmetic;
  min_prime_index      = opts.n_primes_in_primorial; 
  poolshare            = opts.poolshare;
  primes               = opts.primes->ptr;
  two_inverses         = opts.two_inverses;
  chain_length         = opts.chain_length;
  extensions           = opts.n_sieve_extensions;
  layers               = extensions + chain_length;
  max_prime_index      = opts.max_prime_index;
  cache_bits           = opts.cachebits;
  sieve_words          = opts.sieve_words;
  sieve_size           = opts.sievesize;
  candidate_bytes      = sizeof(sieve_t) * sieve_words;
  bit_half             = sieve_size  / 2;
  word_half            = sieve_words / 2;
  cache_words          = word_index(cache_bits);
  cache_bytes          = byte_index(cache_bits); 
  
}

/**
 * frees sieve globals
 */
void free_sieve_globals() {
  
  mpz_clear(mpz_fixed_hash_multiplier);
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
         (int) chain_length,
         (int) poolshare,
         (int) candidate_bytes,
         (int) sieve_size,
         (int) extensions,
         (int) layers,
         (int) max_prime_index,
         (int) min_prime_index,
         (int) cache_bits,
         (int) sieve_words,
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

  printf("mpz_test_origin:      ");
  mpz_out_str(stdout, 10, sieve->mpz_test_origin);
  printf("\n");

  printf("mpz_fixed_hash_multiplier: ");
  mpz_out_str(stdout, 10, mpz_fixed_hash_multiplier);
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

  memset(sieve->cc1, 0, candidate_bytes);
  memset(sieve->cc2, 0, candidate_bytes);
  memset(sieve->twn, 0, candidate_bytes);
  memset(sieve->all, 0, candidate_bytes);

  memset(sieve->ext_cc1, 0, candidate_bytes * extensions);
  memset(sieve->ext_cc2, 0, candidate_bytes * extensions);
  memset(sieve->ext_twn, 0, candidate_bytes * extensions);

  memset(sieve->cc1_layer, 0, candidate_bytes);
  memset(sieve->cc2_layer, 0, candidate_bytes);

  /* for cc1 and cc2 chains, for each layer */
  memset(sieve->cc1_muls, 0xFF, sizeof(uint32_t *) * 
                                layers * 
                                max_prime_index);

  memset(sieve->cc2_muls, 0xFF, sizeof(uint32_t *) * 
                                layers * 
                                max_prime_index);

}

/**
 * initializes a given sieve for the first
 */
void init_sieve(Sieve *sieve) {

  memset(sieve, 0, sizeof(Sieve));
  sieve_set_header(sieve, opts.header);
    
  mpz_init(sieve->mpz_test_origin);
  mpz_init(sieve->mpz_multiplier);
  mpz_init(sieve->mpz_reminder);
  mpz_init(sieve->mpz_hash);
  mpz_init(sieve->mpz_tmp);

  sieve->cc1 = (sieve_t *) malloc(candidate_bytes);
  sieve->cc2 = (sieve_t *) malloc(candidate_bytes);
  sieve->twn = (sieve_t *) malloc(candidate_bytes);
  sieve->all = (sieve_t *) malloc(candidate_bytes);

  sieve->cc1_layer = (sieve_t *) malloc(candidate_bytes);
  sieve->cc2_layer = (sieve_t *) malloc(candidate_bytes);

  sieve->ext_cc1 = (sieve_t *) malloc(candidate_bytes * extensions);
  sieve->ext_cc2 = (sieve_t *) malloc(candidate_bytes * extensions);
  sieve->ext_twn = (sieve_t *) malloc(candidate_bytes * extensions);
  sieve->ext_all = (sieve_t *) malloc(candidate_bytes * extensions);

  /* for cc1 and cc2 chains, for each layer */
  sieve->cc1_muls = (uint32_t *) malloc(sizeof(uint32_t *) * 
                                       layers * 
                                       max_prime_index);

  sieve->cc2_muls = (uint32_t *) malloc(sizeof(uint32_t *) * 
                                        layers * 
                                        max_prime_index);

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
  free(sieve->ext_cc1);
  free(sieve->ext_cc2);
  free(sieve->ext_twn);
  free(sieve->cc1_layer);
  free(sieve->cc2_layer);

  clear_test_params(&sieve->test_params);

  mpz_clear(sieve->mpz_test_origin);
  mpz_clear(sieve->mpz_multiplier);
  mpz_clear(sieve->mpz_reminder);
  mpz_clear(sieve->mpz_hash);
  mpz_clear(sieve->mpz_tmp);
}

/**
 * sieves all primes in the given intervall, and layer (cache optimation)
 * for the given candidates array
 */
static void sieve_from_to(sieve_t *const candidates,
                                 uint32_t *const multipliers,
                                 const uint32_t start,
                                 const uint32_t end,
                                 const uint32_t layer) {
  /* wipe the array */
  memset(candidates + word_index(start), 0, cache_bytes);

  uint32_t i;
  for (i = min_prime_index; i < max_prime_index; i++) {

    /* current prime */
    const uint32_t prime = primes[i];
    
    /* current factor */
    uint32_t factor = multipliers[i * layers + layer];

    /* adjust factor */
    if (factor < start)
      factor += (start - factor + prime - 1) / prime * prime;

#ifdef USE_ROTATE
    sieve_t word = bit_word(factor);
    const uint32_t rotate = bit_index(prime);

    /* progress the given range of the sieve */
    for (; factor < end; factor += prime) {

      word_at(candidates, factor) |= word;
      word = (word << rotate) | (word >> (word_bits - rotate));
    }
#else

    /* progress the given range of the sieve */
    for (; factor < end; factor += prime) {

      word_at(candidates, factor) |= bit_word(factor);
    }

#endif

    /* save the factor for the next round */
    multipliers[i * layers + layer] = factor;
  }
}


/**
 * test the found candidates 
 */
static inline void test_candidates(Sieve *const sieve, 
                                   const sieve_t *const cc1,
                                   const sieve_t *const twn,
                                   const sieve_t *const all,
                                   const mpz_t mpz_primorial,
                                   const uint32_t extension) {

  SieveStats *const stats = &sieve->stats;

  uint32_t i;
  for (i = ((extension > 0) ? (sieve_words / 2) : 0); 
       sieve->active && i < sieve_words; 
       i++) {

    
    const sieve_t word = all[i];

    /* skipp a word if no candidates in it */
    if (word == word_max) continue; 


    sieve_t n, bit;
    for (n = 1, bit = 0; n != 0; n <<= 1, bit++) {

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
          
          difficulty = twn_chain_test(sieve->mpz_test_origin,
                                      &sieve->test_params); 

          stats->twn[chain_length(difficulty)]++;
          type = BI_TWIN_CHAIN;

        /* cc1 candidate */
        } else if ((cc1[i] & n) == 0) {

          difficulty = cc1_chain_test(sieve->mpz_test_origin,
                                      &sieve->test_params);

          stats->cc1[chain_length(difficulty)]++;
          type = FIRST_CUNNINGHAM_CHAIN;

        /* cc2 candidate */
        } else {
        
          difficulty = cc2_chain_test(sieve->mpz_test_origin,
                                      &sieve->test_params);

          stats->cc2[chain_length(difficulty)]++;
          type = SECOND_CUNNINGHAM_CHAIN;
        }
        
        if (chain_length(difficulty) >= poolshare) {
          
          mpz_mul_ui(sieve->mpz_multiplier, 
                     mpz_fixed_hash_multiplier, 
                     (word_bits * i) << extension);

          mpz_to_ary(sieve->header.primemultiplier, sieve->mpz_multiplier);
          submit_share(&sieve->header, type, difficulty); 
        }
      }
    }
  }
} 

/**
 * calculates the multipliers for sieving
 */
static inline void calc_multipliers(Sieve *const sieve, 
                                    const mpz_t mpz_primorial) {

#ifdef PRINT_TIME
  uint64_t start_time = gettime_usec();
#endif

  uint32_t *const cc1_muls       = sieve->cc1_muls;
  uint32_t *const cc2_muls       = sieve->cc2_muls;

  /* generate the multiplicators for the first layer first */
  uint32_t i;
  for (i = min_prime_index; 
       sieve->active && i < max_prime_index; 
       i++) {

    /* current prime */
    const uint32_t prime = primes[i];

    /* modulo = primorial % prime */
    const uint32_t modulo = (uint32_t) mpz_tdiv_ui(mpz_primorial, prime);

    /* nothing in the sieve is divisible by this prime */
    if (modulo == 0) continue;

    uint32_t factor = invert(modulo, prime);
    const uint32_t two_inverse = two_inverses[i];

    
    const uint32_t offset = layers * i;
    uint32_t l;

    if (i < int64_arithmetic) {

      for (l = 0; l < layers; l++) {
     
        cc1_muls[offset + l] = factor;
        cc2_muls[offset + l] = prime - factor;
     
        /* calc factor for the next number in chain */
        factor = (factor * two_inverse) % prime;
      }
    } else {

      for (l = 0; l < layers; l++) {
     
        cc1_muls[offset + l] = factor;
        cc2_muls[offset + l] = prime - factor;
     
        /* calc factor for the next number in chain */
        factor = (uint32_t) (((uint64_t) factor) * 
                             ((uint64_t) two_inverse)) % ((uint64_t) prime);
      }
    }
  }

#ifdef PRINT_TIME
  printf("[DD] calulating mulls: %" PRIu64 "\n", gettime_usec() - start_time);
#endif

  /* run test if DEBUG is enabeld */
  check_mulltiplier(mpz_primorial,    
                    cc1_muls,         
                    cc2_muls,         
                    sieve_size,
                    layers,           
                    primes,           
                    min_prime_index,         
                    max_prime_index);

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
  
  /* check the primes */
  check_primes(primes, max_prime_index);

#ifdef PRINT_TIME
  uint64_t start_time = gettime_usec();
#endif

  uint32_t *const cc1_muls       = sieve->cc1_muls;
  uint32_t *const cc2_muls       = sieve->cc2_muls;
  sieve_t *const twn             = sieve->twn;
  sieve_t *const cc2             = sieve->cc2;
  sieve_t *const cc1             = sieve->cc1;
  sieve_t *const all             = sieve->all;
  sieve_t *const cc1_layer       = sieve->cc1_layer;
  sieve_t *const cc2_layer       = sieve->cc2_layer;
  sieve_t *const ext_twn         = sieve->ext_twn;
  sieve_t *const ext_cc2         = sieve->ext_cc2;
  sieve_t *const ext_cc1         = sieve->ext_cc1;
  sieve_t *const ext_all         = sieve->ext_all;
  
  /* calculate the multipliers first */
  calc_multipliers(sieve, mpz_primorial);

  /* calculate the wi-twin cc1 and cc2 layers */
  const uint32_t twn_cc1_layers = (chain_length + 1) / 2 - 1;
  const uint32_t twn_cc2_layers = chain_length       / 2 - 1;
  uint32_t l, w, e;

  uint32_t word_start, word_end, bit_start, bit_end;

  for (word_start = 0, 
       bit_start  = 0, 
       word_end   = cache_words,
       bit_end    = cache_bits;
       sieve->active && bit_start < bit_half;
       word_start += cache_words,
       word_end   += cache_words,
       bit_start  += cache_bits,
       bit_end    += cache_bits) {

    for (l = 0; sieve->active && l < chain_length; l++) {

#ifdef PRINT_CACHE_TIME      
      uint64_t cache_time = gettime_usec();
#endif
      sieve_from_to(cc2_layer, cc2_muls, bit_start, bit_end, l);  
#ifdef PRINT_CACHE_TIME
      printf("[DD] cache time: %" PRIu64 "\n", gettime_usec() - cache_time);
#endif
      sieve_from_to(cc1_layer, cc1_muls, bit_start, bit_end, l);  

      for (w = word_start; w < word_end; w++) {
        cc2[w] |= cc2_layer[w];
        cc1[w] |= cc1_layer[w];
      }

      /* copy layers to the twn candidates */
      if (l == twn_cc2_layers) 
        memcpy(twn + word_start, cc2 + word_start, cache_bytes);

      /* applay layers to the twn candidates */
      if (l == twn_cc1_layers) 
        for (w = word_start; w < word_end; w++)
          twn[w] |= cc1[w];
    }
  }

  for (word_start = word_half, 
       bit_start  = bit_half, 
       word_end   = word_half + cache_words,
       bit_end    = bit_half  + cache_bits;
       sieve->active && bit_start < sieve_size;
       word_start += cache_words,
       word_end   += cache_words,
       bit_start  += cache_bits,
       bit_end    += cache_bits) {

    for (l = 0; sieve->active && l < layers; l++) {

     // uint64_t cache_time = gettime_usec();
      sieve_from_to(cc2_layer, cc2_muls, bit_start, bit_end, l);  
     // printf("[DD] cache time: %" PRIu64 "\n", gettime_usec() - cache_time);
      sieve_from_to(cc1_layer, cc1_muls, bit_start, bit_end, l);  


      if (l < chain_length) {

        for (w = word_start; w < word_end; w++) {
          cc2[w] |= cc2_layer[w];
          cc1[w] |= cc1_layer[w];
        }
      }

      /* copy layers to the twn candidates */
      if (l == twn_cc2_layers) 
        memcpy(twn + word_start, cc2 + word_start, cache_bytes);

      /* applay layers to the twn candidates */
      if (l == twn_cc1_layers) 
        for (w = word_start; w < word_end; w++)
          twn[w] |= cc1[w];


      /* applay layers to the extensions */
      for (e = 0; sieve->active && e < extensions; e++) {
        
        uint32_t ext_offset = e * sieve_words;
        uint32_t ext_layer  = l - (e + 1);
     
        if (e < l && l <= e + chain_length) {
     
          sieve_t *ptr_cc2 = ext_cc2 + ext_offset;
          sieve_t *ptr_cc1 = ext_cc1 + ext_offset;
     
          for (w = word_start; w < word_end; w++) {
            ptr_cc2[w] |= cc2_layer[w];
            ptr_cc1[w] |= cc1_layer[w];
          }
        }

        /* copy layers to the extended twn candidates */
        if (ext_layer == twn_cc2_layers) {
          uint32_t offset = word_start + ext_offset;
          memcpy(ext_twn + offset, ext_cc2 + offset, cache_bytes);
        }

        /* copy layers to the extended twn candidates */ 
        if (ext_layer == twn_cc1_layers) {

          sieve_t *ptr_cc1 = ext_cc1 + ext_offset;
          sieve_t *ptr_twn = ext_twn + ext_offset;
     
          for (w = word_start; w < word_end; w++)
            ptr_twn[w] |= ptr_cc1[w];
        }
      }
    }
  }

#ifdef CHECK_SIEVE
  /* reclaculate the multiliers for easy sieveing */
  calc_multipliers(sieve, mpz_primorial);
#endif 

  check_sieve(cc1,                     
              cc2,                     
              twn,                     
              ext_cc1,                 
              ext_cc2,                 
              ext_twn,                 
              cc1_muls,                
              cc2_muls,                
              chain_length,            
              sieve_words,
              extensions,              
              layers,
              primes,                  
              min_prime_index,               
              max_prime_index);

  /* create the final set of candidates */
  uint32_t i;
  for (i = 0; i < sieve_words; i++)
    all[i] = cc1[i] & cc2[i] & twn[i];

  /* mark factor 0 as composite */
  all[0] |= (sieve_t) 1;


  /* create the final set of extended candidates */
  for (e = 0; sieve->active && e < extensions; e++) {
    
    sieve_t *ptr_cc1 = ext_cc1 + e * sieve_words;
    sieve_t *ptr_cc2 = ext_cc2 + e * sieve_words;
    sieve_t *ptr_twn = ext_twn + e * sieve_words;
    sieve_t *ptr_all = ext_all + e * sieve_words;

    /* create the final set of candidates */
    for (i = sieve_words / 2; i < sieve_words; i++) 
      ptr_all[i] = ptr_cc1[i] & ptr_cc2[i] & ptr_twn[i];

    /* mark factor 0 as composite */
    ptr_all[0] |= (sieve_t) 1;
   }

#ifdef PRINT_TIME
  printf("[DD] sieveing: %" PRIu64 "\n", gettime_usec() - start_time);
  start_time = gettime_usec();
#endif

  /* check sieve out put if DEBUG is enabeld */
  chech_candidates(mpz_primorial,     
                   all,               
                   cc1,               
                   twn,               
                   chain_length,      
                   0,         
                   sieve_words,
                   &sieve->test_params);

  /* run the feramt test on the remaining candidates */
  test_candidates(sieve, cc1, twn, all, mpz_primorial, 0); //TODO wite a test for _test_candidates

  /* test extended candidates */
  for (e = 0; sieve->active && e < extensions; e++) {
    
    sieve_t *ptr_cc1 = ext_cc1 + e * sieve_words;
    sieve_t *ptr_twn = ext_twn + e * sieve_words;
    sieve_t *ptr_all = ext_all + e * sieve_words;

    /* check sieve out put if DEBUG is enabeld */
    chech_candidates(mpz_primorial,     // TODO stimmt so nicht mehr
                     ptr_all,               
                     ptr_cc1,               
                     ptr_twn,               
                     chain_length,      
                     e + 1,         
                     sieve_words,
                     &sieve->test_params);

    /* run the feramt test on the remaining candidates */
    test_candidates(sieve, ptr_cc1, ptr_twn, ptr_all, mpz_primorial, e + 1);
  }
#ifdef PRINT_TIME
  printf("[DD] testing : %" PRIu64 "\n", gettime_usec() - start_time);
#endif

  /* check candidate ratio if DEBUG is enabeld */
  check_ratio(&sieve->stats);
}
