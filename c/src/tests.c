/**
 * implementation of some test to check the accuracy of the alorithm
 */
#ifdef DEBUG

#include <stdio.h>
#include <gmp.h>

#include "main.h"

#ifdef CHECK_MULLTIPLIER
/**
 * this checks that the calculated multiplier:
 */
char check_mulltiplier(const mpz_t mpz_primorial,
                       const uint32_t *const cc1_muls, 
                       const uint32_t *const cc2_muls,
                       const uint32_t sieve_size,
                       const uint32_t layers,
                       const uint32_t *const primes,
                       const uint32_t min_prime,
                       const uint32_t max_prime) {

  mpz_t mpz_composite;
  mpz_init(mpz_composite);

  uint32_t i;

  /* test the cc1 multiplier */
  for (i = min_prime; i < max_prime; i++) {
    
    printf("[DD] testing prime cc1 multiplier %" PRIu32 "   \r", max_prime - i);

    /* skipp if there is no multiplier for this prime avilabe */
    if (cc1_muls[i * layers] == UINT32_MAX) continue;

    uint32_t l;
    for (l = 0; l < layers; l++) {

      uint32_t factor = cc1_muls[i * layers + l];

      /* progress the given range of the sieve */
      for (; factor < sieve_size; factor += primes[i]) {

        /* calculate the composite prime candidate */
        mpz_mul_ui(mpz_composite, mpz_primorial, factor);
      
        /* calculate the n-chain candidate */
        if (l > 0)
         mpz_mul_2exp(mpz_composite, mpz_composite, l);
 
        /* -1 for cc1 candidate primes */
        mpz_sub_ui(mpz_composite, mpz_composite, 1);
 
        /* now test if the prime candidate is divisible by the prime */
        if (mpz_tdiv_ui(mpz_composite, primes[i]) != 0) {
          
          printf("[EE] multiplier test failed for cc1 candidate:\n"
                 "[EE] multiplier: %" PRIu32 "\n"
                 "[EE] layer:      %" PRIu32 "\n"
                 "[EE] prime:      %" PRIu32 "\n"
                 "[EE] primorial:  ",
                 cc1_muls[i * layers + l],
                 l,
                 primes[i]);
 
          mpz_out_str(stdout, 10, mpz_primorial);
          printf("\n");
 
          mpz_clear(mpz_composite);
 
          return -1;
        }
      }
    }
  }

  /* test the cc2 multiplier */
  for (i = min_prime; i < max_prime; i++) {

    printf("[DD] testing prime cc2 multiplier %" PRIu32 "   \r", max_prime - i);

    /* skipp if there is no multiplier for this prime avilabe */
    if (cc2_muls[i * layers] == UINT32_MAX) continue;

    uint32_t l;
    for (l = 0; l < layers; l++) {

      uint32_t factor = cc2_muls[i * layers + l];

      /* progress the given range of the sieve */
      for (; factor < sieve_size; factor += primes[i]) {

        /* calculate the composite prime candidate */
        mpz_mul_ui(mpz_composite, mpz_primorial, factor);
     
        /* calculate the n-chain candidate */
        if (l > 0)
         mpz_mul_2exp(mpz_composite, mpz_composite, l);
       
        /* +1 for cc2 candidate primes */
        mpz_add_ui(mpz_composite, mpz_composite, 1);
       
        /* now test if the prime candidate is divisible by the prime */
        if (mpz_tdiv_ui(mpz_composite, primes[i]) != 0) {
          
          printf("[EE] multiplier test failed for cc2 candidate:\n"
                 "[EE] multiplier: %" PRIu32 "\n"
                 "[EE] layer:      %" PRIu32 "\n"
                 "[EE] prime:      %" PRIu32 "\n"
                 "[EE] primorial:  ",
                 cc1_muls[i * layers + l],
                 l,
                 primes[i]);
       
          mpz_out_str(stdout, 10, mpz_primorial);
          printf("\n");
          
          mpz_clear(mpz_composite);
       
          return -1;
        }
      }
    }
  }

  mpz_clear(mpz_composite);

  printf("\r[DD] prime multiplier tested succesfull     \n");

  return 0;
}
#endif

#ifdef CHECK_CANDIDATES
/**
 * test if an candidate array was soeved correctly
 */
char chech_candidates(const mpz_t mpz_primorial,
                      const sieve_t *const all,
                      const sieve_t *const cc1,
                      const sieve_t *const twn,
                      const uint32_t chain_length,
                      const uint32_t extension,
                      const uint32_t sieve_words,
                      TestParams *const test_params) {

  return 0; // disabled

  mpz_t mpz_origin;
  mpz_init(mpz_origin);

  uint32_t i;
  for (i = (extension ? (sieve_words / 2) : 0); 
       i < sieve_words; 
       i++) {

    printf("[DD] cheching sieve for extension %" PRIu32 ": %" PRIu32 "  \r",
           extension, sieve_words - i);
    
    const sieve_t word = all[i];
    
    sieve_t n, bit;
    for (n = 1, bit = 1; n != 0; n <<= 1, bit++) {

      /* fond an sieved index */
      if ((word & n) == 1) {

        /* origins = primorial * index << extension */
        mpz_mul_ui(mpz_origin, 
                   mpz_primorial, 
                   (word_bits * i + bit) << extension);

        uint32_t difficulty;
        char     *type;

        /* bi-twin candidate */
        if ((twn[i] & n) == 0) {
          
          difficulty = twn_chain_test(mpz_origin, test_params); 

          type = BI_TWIN_CHAIN;
        /* cc1 candidate */
        } else if ((cc1[i] & n) == 0) {

          difficulty = cc1_chain_test(mpz_origin, test_params);

          type = FIRST_CUNNINGHAM_CHAIN;
        /* cc2 candidate */
        } else {
        
          difficulty = cc2_chain_test(mpz_origin, test_params);

          type = SECOND_CUNNINGHAM_CHAIN;
        }

        /**
         * check that the sieved candidate was not a cunningham
         * candidate of chain_length 
         */
        if (chain_length(difficulty) >= chain_length) {
          
          printf("[EE] multiplier test failed for %s candidate:\n"
                 "[EE] multiplier: %" PRISIEVET "\n"
                 "[EE] origin:     ",
                 type,
                 (word_bits * i + bit) << extension);

          mpz_out_str(stdout, 10, mpz_origin);
          printf("\n");
          mpz_clear(mpz_origin);


          return -1;
        }
      }
    }
  }
  
  mpz_clear(mpz_origin);

  printf("\r[DD] succesfull tested sieve for extension %" PRIu32 "     \n",
           extension);

  return 0;
}
#endif

#ifdef CHECK_RATIO
/**
 * tests that the ratio of cc1, cc2 and twn candidates are not to different
 */
char check_ratio(const SieveStats *const stats) {

  uint64_t cc1 = 0, cc2 = 0, twn = 0;

  uint32_t i;
  for (i = 0; i < MAX_CHAIN_LENGTH; i++) {
    twn += stats->twn[i];
    cc2 += stats->cc2[i];
    cc1 += stats->cc1[i];
  }

  if (twn + cc2 + cc1 > 10000 &&
      (twn / cc1 > 1 ||
       twn / cc2 > 1 ||
       cc2 / cc1 > 1 ||
       cc2 / twn > 1 ||
       cc1 / cc2 > 1 ||
       cc1 / twn > 1)) {

    
    printf("[EE] unusual candidate ratio: twn %" 
           PRIu64 " cc2 %" PRIu64 " cc1 %" PRIu64 "\n",
           twn,
           cc2,
           cc1);

    return -1;
  }

  printf("[DD] succesfully tested sieve candidate ratio\n");

  return 0;
}
#endif

#ifdef CHECK_SIEVE
/**
 * sieves all primes in the given layer 
 */
static void sieve_from_to(sieve_t *const candidates,
                          const uint32_t *const multipliers,
                          const uint32_t sieve_size,
                          const uint32_t layers,
                          const uint32_t layer,
                          const uint32_t *const primes,
                          const uint32_t min_prime,
                          const uint32_t max_prime) {

  /* wipe the array */
  memset(candidates, 0, sieve_size / word_bits);

  uint32_t i;
  for (i = min_prime; i < max_prime; i++) {

    /* current prime */
    const uint32_t prime = primes[i];
    
    /* current factor */
    uint32_t factor = multipliers[i * layers + layer];

    /* progress the given range of the sieve */
    for (; factor < sieve_size; factor += prime) {

      candidates[factor / word_bits] |= (((sieve_t) 1) << (factor % word_bits));
    }
  }
}

/**
 * checks if an given arrays in the given intervall are equal
 */
char ary_eql(const sieve_t *const ary1,
             const sieve_t *const ary2,
             uint32_t start,
             uint32_t end) {

  uint32_t i;
  for (i = start; i < end; i++) {
    
    if (ary1[i] != ary2[i])
      return 0;
  }
  
  return 1;
}

/**
 * easy sieveing without cache optimation and stuff to check
 * the high performace version
 */
#ifdef USE_EASY_SIEVE
char check_sieve(sieve_t *const cc1,
                 sieve_t *const cc2,
                 sieve_t *const twn,
                 sieve_t *const ext_cc1,
                 sieve_t *const ext_cc2,
                 sieve_t *const ext_twn,
                 const uint32_t *const cc1_muls,
                 const uint32_t *const cc2_muls,
                 const uint32_t chain_length,
                 const uint32_t sieve_words,
                 const uint32_t extensions,
                 const uint32_t layers,
                 const uint32_t *const primes,
                 const uint32_t min_prime,
                 const uint32_t max_prime) {
#else
char check_sieve(const sieve_t *const cc1,
                 const sieve_t *const cc2,
                 const sieve_t *const twn,
                 const sieve_t *const ext_cc1,
                 const sieve_t *const ext_cc2,
                 const sieve_t *const ext_twn,
                 const uint32_t *const cc1_muls,
                 const uint32_t *const cc2_muls,
                 const uint32_t chain_length,
                 const uint32_t sieve_words,
                 const uint32_t extensions,
                 const uint32_t layers,
                 const uint32_t *const primes,
                 const uint32_t min_prime,
                 const uint32_t max_prime) {
#endif                 

  sieve_t *cc1_cpy = calloc(sizeof(sieve_t), sieve_words);
  sieve_t *cc2_cpy = calloc(sizeof(sieve_t), sieve_words);
  sieve_t *twn_cpy = calloc(sizeof(sieve_t), sieve_words);
  sieve_t *ext_cc1_cpy = calloc(sizeof(sieve_t), sieve_words * extensions);
  sieve_t *ext_cc2_cpy = calloc(sizeof(sieve_t), sieve_words * extensions);
  sieve_t *ext_twn_cpy = calloc(sizeof(sieve_t), sieve_words * extensions);

  sieve_t *cc1_layer = calloc(sizeof(sieve_t), sieve_words);
  sieve_t *cc2_layer = calloc(sizeof(sieve_t), sieve_words);

  const uint32_t sieve_size = sieve_words * word_bits;
  const uint32_t twn_cc1_layers = (chain_length + 1) / 2;
  const uint32_t twn_cc2_layers = chain_length       / 2;

  uint32_t w, e, l;
  for (l = 0; l < layers; l++) {
    
    /* calculate layer i */
    sieve_from_to(cc1_layer, cc1_muls, sieve_size, layers, l, primes, min_prime, max_prime);
    sieve_from_to(cc2_layer, cc2_muls, sieve_size, layers, l, primes, min_prime, max_prime);

    /* applay the layer */
    if (l < chain_length) {
      
      if (l < twn_cc2_layers) {

        for (w = 0; w < sieve_words; w++) { 

          cc1_cpy[w] |= cc1_layer[w];
          cc2_cpy[w] |= cc2_layer[w];
          twn_cpy[w] |= cc1_layer[w] | cc2_layer[w];
        }
      } else if (l < twn_cc1_layers) {
        
        for (w = 0; w < sieve_words; w++) { 

          cc1_cpy[w] |= cc1_layer[w];
          cc2_cpy[w] |= cc2_layer[w];
          twn_cpy[w] |= cc1_layer[w];
        }
      } else {

        for (w = 0; w < sieve_words; w++) { 

          cc1_cpy[w] |= cc1_layer[w];
          cc2_cpy[w] |= cc2_layer[w];
        }
      }
    }


    for (e = 0; e < extensions; e++) {
      
      uint32_t layer_offset = e + 1;

      if (l >= layer_offset && l < chain_length + layer_offset) {
        
        uint32_t ext_layer = l - layer_offset;
        sieve_t *ext_ptr_cc1 = ext_cc1_cpy + sieve_words * e;
        sieve_t *ext_ptr_cc2 = ext_cc2_cpy + sieve_words * e;
        sieve_t *ext_ptr_twn = ext_twn_cpy + sieve_words * e;

        if (ext_layer < twn_cc2_layers) {
       
          for (w = 0; w < sieve_words; w++) { 
       
            ext_ptr_cc1[w] |= cc1_layer[w];
            ext_ptr_cc2[w] |= cc2_layer[w];
            ext_ptr_twn[w] |= cc1_layer[w] | cc2_layer[w];
          }
        } else if (ext_layer < twn_cc1_layers) {
          
          for (w = 0; w < sieve_words; w++) { 
       
            ext_ptr_cc1[w] |= cc1_layer[w];
            ext_ptr_cc2[w] |= cc2_layer[w];
            ext_ptr_twn[w] |= cc1_layer[w];
          }
        } else {
       
          for (w = 0; w < sieve_words; w++) { 
       
            ext_ptr_cc1[w] |= cc1_layer[w];
            ext_ptr_cc2[w] |= cc2_layer[w];
          } 
        }
      }
    }
  }

  char ret = 0;
  
  /* check original sieveing */
  if (!ary_eql(cc1, cc1_cpy, 0, sieve_words)) {
    //printf("[EE] cc1 not equal with easy sieving!\n");
    ret = -1;;
  }
  
  if (!ary_eql(cc2, cc2_cpy, 0, sieve_words)) {
    //printf("[EE] cc2 not equal with easy sieving!\n");
    ret = -1;;
  }
  
  if (!ary_eql(twn, twn_cpy, 0, sieve_words)) {
    //printf("[EE] twn not equal with easy sieving!\n");
    ret = -1;;
  }
  
  for (e = 0; e < extensions; e++) {
    
    const sieve_t *const ext_ptr_cc1 = ext_cc1 + sieve_words * e;
    const sieve_t *const ext_ptr_cc2 = ext_cc2 + sieve_words * e;
    const sieve_t *const ext_ptr_twn = ext_twn + sieve_words * e;

    sieve_t *ext_ptr_cc1_cpy = ext_cc1_cpy + sieve_words * e;
    sieve_t *ext_ptr_cc2_cpy = ext_cc2_cpy + sieve_words * e;
    sieve_t *ext_ptr_twn_cpy = ext_twn_cpy + sieve_words * e;

    if (!ary_eql(ext_ptr_cc1, ext_ptr_cc1_cpy, sieve_words / 2, sieve_words)) {
      //printf("[EE] ext_cc1[%" PRIu32 "] not equal with easy sieving!\n", e);
      ret = -1;;
    }

    if (!ary_eql(ext_ptr_cc2, ext_ptr_cc2_cpy, sieve_words / 2, sieve_words)) {
      //printf("[EE] ext_cc2[%" PRIu32 "] not equal with easy sieving!\n", e);
      ret = -1;;
    }

    if (!ary_eql(ext_ptr_twn, ext_ptr_twn_cpy, sieve_words / 2, sieve_words)) {
      //printf("[EE] ext_twn[%" PRIu32 "] not equal with easy sieving!\n", e);
      ret = -1;;
    }
  }

#ifdef USE_EASY_SIEVE
  if (ret != 0) {
    printf("cpy..\n");
    memcpy(cc1, cc1_cpy, sizeof(sieve_t) * sieve_words);
    memcpy(cc2, cc2_cpy, sizeof(sieve_t) * sieve_words);
    memcpy(twn, twn_cpy, sizeof(sieve_t) * sieve_words);

    memcpy(ext_cc1, ext_cc1_cpy, sizeof(sieve_t) * sieve_words * extensions);
    memcpy(ext_cc2, ext_cc2_cpy, sizeof(sieve_t) * sieve_words * extensions);
    memcpy(ext_twn, ext_twn_cpy, sizeof(sieve_t) * sieve_words * extensions);
  }
#endif

  free(cc1_cpy);
  free(cc2_cpy);
  free(twn_cpy);
  free(ext_cc1_cpy);
  free(ext_cc2_cpy);
  free(ext_twn_cpy);

  free(cc1_layer);
  free(cc2_layer);

  if (ret == 0)
    printf("[DD] Succesfully tested sieveing\n");

  return ret;
}
#endif 

#ifdef CHECK_PRIMES
char check_primes(const uint32_t *const primes, const uint32_t len) {


  if (primes[0] != 2) {
    printf("[EE] primes[0] != 2\n");
    return -1;
  }

  uint32_t i;
  for (i = 1; i < len; i++) {
    
    uint32_t max = sqrt(primes[i]) + 1;
    for (; max > 1; max--) {
      
      if (primes[i] % max == 0) {
        printf("[EE] primes[%" PRIu32 "] = %" PRIu32 "is not a prime!!!\n",
               i,
               primes[i]);
        return -1;
      }
    }
  }

  printf("[DD] succesfully checkt primes\n");
  return 0;
}
#endif

#endif /* DEBUG */
