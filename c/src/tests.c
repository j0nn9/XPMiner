/**
 * implementation of some test to check the accuracy of the alorithm
 */
#ifdef DEBUG

#include <math.h>
#include <stdio.h>
#include <gmp.h>

#include "main.h"

#ifdef CHECK_MULLTIPLIER
/**
 * this checks the calculated multiplier (i), working
 * as expected by testing if all sieve enties (n * p + i) * H +/- 1
 * to be divisieble by i
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

          error_msg("[EE] multiplier test failed for cc1 candidate:\n"
                    "[EE] multiplier:  %" PRIu32 "\n"
                    "[EE] layer:       %" PRIu32 "\n"
                    "[EE] prime:       %" PRIu32 "\n"
                    "[EE] primorial:   ",
                    cc1_muls[i * layers + l],
                    l,
                    primes[i]);
 
          print_mpz(mpz_primorial);
          error_msg("\n");
 
          mpz_clear(mpz_composite);
 
          return -1;
        }
      }
    }
  }

  /* test the cc2 multiplier */
  for (i = min_prime; i < max_prime; i++) {

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
          
          error_msg("[EE] multiplier test failed for cc2 candidate:\n"
                    "[EE] multiplier: %" PRIu32 "\n"
                    "[EE] layer:      %" PRIu32 "\n"
                    "[EE] prime:      %" PRIu32 "\n"
                    "[EE] primorial:  ",
                    cc1_muls[i * layers + l],
                    l,
                    primes[i]);
       
          print_mpz(mpz_primorial);
          error_msg("\n");
          
          mpz_clear(mpz_composite);
       
          return -1;
        }
      }
    }
  }

  mpz_clear(mpz_composite);

  error_msg("[DD] prime multiplier tested succesfull\n");

  return 0;
}
#endif

#ifdef CHECK_CANDIDATES
/**
 * test if an candidate array was sieved correctly
 * (testing that all sieved indexes are not chains of lenght chain_length)
 */
char check_candidates(const mpz_t mpz_primorial,
                      const sieve_t *const all,
                      const sieve_t *const cc1,
                      const sieve_t *const twn,
                      const uint32_t chain_length,
                      const uint32_t extension,
                      const uint32_t sieve_words,
                      TestParams *const test_params) {

  mpz_t mpz_origin;
  mpz_init(mpz_origin);

  uint32_t i;
  for (i = (extension ? (sieve_words / 2) : 0); 
       i < sieve_words; 
       i++) {

    const sieve_t word = all[i];
    
    sieve_t n, bit;
    for (n = 1, bit = 1; n != 0; n <<= 1, bit++) {

      /* fond an sieved index */
      if ((word & n) == 1) {

        /* origins = primorial * index << extension */
        mpz_mul_ui(mpz_origin, 
                   mpz_primorial, 
                   (word_bits * i + bit) << extension);

        uint32_t chain;
        char     type;

        /* bi-twin candidate */
        if ((twn[i] & n) == 0) {
          
          chain = twn_chain_test(mpz_origin, test_params); 

          type = BI_TWIN_CHAIN;
        /* cc1 candidate */
        } else if ((cc1[i] & n) == 0) {

          chain = cc1_chain_test(mpz_origin, test_params);

          type = FIRST_CUNNINGHAM_CHAIN;
        /* cc2 candidate */
        } else {
        
          chain = cc2_chain_test(mpz_origin, test_params);

          type = SECOND_CUNNINGHAM_CHAIN;
        }

        /**
         * check that the sieved candidate was not a cunningham
         * candidate of chain_length 
         */
        if (chain >= chain_length) {
          
          error_msg("[EE] multiplier test failed for %s candidate:\n"
                    "[EE] multiplier: %" PRISIEVET "\n"
                    "[EE] origin:     ",
                    type,
                    (word_bits * i + bit) << extension);

          print_mpz(mpz_origin);
          error_msg("\n");
          mpz_clear(mpz_origin);


          return -1;
        }
      }
    }
  }
  
  mpz_clear(mpz_origin);

  error_msg("[DD] succesfull tested sieve for extension %" PRIu32 "\n",
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

    
    error_msg("[EE] unusual candidate ratio: twn %" 
              PRIu64 " cc2 %" PRIu64 " cc1 %" PRIu64 "\n",
              twn,
              cc2,
              cc1);

    return -1;
  }

  error_msg("[DD] succesfully tested sieve candidate ratio\n");

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
  memset(candidates, 0, sieve_size / 8);

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
char check_sieve(const sieve_t *const cc1,
                 const sieve_t *const cc2,
                 const sieve_t *const twn,
                 const sieve_t *const ext_cc1,
                 const sieve_t *const ext_cc2,
                 const sieve_t *const ext_twn,
                 const uint32_t *const cc1_muls_orig,
                 const uint32_t *const cc2_muls_orig,
                 const uint32_t chain_length,
                 const uint32_t sieve_words,
                 const uint32_t extensions,
                 const uint32_t layers,
                 const uint32_t *const primes,
                 const uint32_t min_prime,
                 const uint32_t max_prime,
                 const mpz_t    mpz_primorial,
                 const uint32_t sieve_size,
                 const uint32_t use_first_half) {

  /* copy and reset the multiplayers */
  uint32_t *cc1_muls = malloc(sizeof(uint32_t) * layers * max_prime);
  uint32_t *cc2_muls = malloc(sizeof(uint32_t) * layers * max_prime);

  memcpy(cc1_muls, cc1_muls_orig, sizeof(uint32_t) * layers * max_prime);
  memcpy(cc2_muls, cc2_muls_orig, sizeof(uint32_t) * layers * max_prime);
  
  uint32_t i;
  for (i = min_prime; i < max_prime; i++) {

    const uint32_t prime  = primes[i];
    const uint32_t offset = layers * i;
    uint32_t l;


    for (l = 0; l < layers; l++) {
    
      if (cc1_muls[offset + l] != UINT32_MAX)
        cc1_muls[offset + l] %= prime;

      if (cc2_muls[offset + l] != UINT32_MAX)
        cc2_muls[offset + l] %= prime;
    }
  }

#ifdef CHECK_MULLTIPLIER
  check_mulltiplier(mpz_primorial,
                    cc1_muls, 
                    cc2_muls,
                    sieve_size,
                    layers,
                    primes,
                    min_prime,
                    max_prime);
#else
  (void) mpz_primorial;
#endif

  sieve_t *cc1_cpy = calloc(sizeof(sieve_t), sieve_words);
  sieve_t *cc2_cpy = calloc(sizeof(sieve_t), sieve_words);
  sieve_t *twn_cpy = calloc(sizeof(sieve_t), sieve_words);
  sieve_t *ext_cc1_cpy = calloc(sizeof(sieve_t), sieve_words * extensions);
  sieve_t *ext_cc2_cpy = calloc(sizeof(sieve_t), sieve_words * extensions);
  sieve_t *ext_twn_cpy = calloc(sizeof(sieve_t), sieve_words * extensions);

  sieve_t *cc1_layer = calloc(sizeof(sieve_t), sieve_words);
  sieve_t *cc2_layer = calloc(sizeof(sieve_t), sieve_words);

  const uint32_t twn_cc1_layers = (chain_length + 1) / 2;
  const uint32_t twn_cc2_layers = chain_length       / 2;
  const uint32_t start = (use_first_half ? 0 : sieve_words / 2);

  uint32_t w, e, l;
  for (l = 0; l < layers; l++) {
    
    /* calculate layer i */
    sieve_from_to(cc1_layer, 
                  cc1_muls, 
                  sieve_size, 
                  layers, 
                  l, 
                  primes, 
                  min_prime, 
                  max_prime);

    sieve_from_to(cc2_layer, 
                  cc2_muls, 
                  sieve_size, 
                  layers, 
                  l, 
                  primes, 
                  min_prime, 
                  max_prime);

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
  if (!ary_eql(cc1, cc1_cpy, start, sieve_words)) {
    error_msg("[EE] cc1 not equal with easy sieving!\n");
    ret = -1;;
  }
  
  if (!ary_eql(cc2, cc2_cpy, start, sieve_words)) {
    error_msg("[EE] cc2 not equal with easy sieving!\n");
    ret = -1;;
  }
  
  if (!ary_eql(twn, twn_cpy, start, sieve_words)) {
    error_msg("[EE] twn not equal with easy sieving!\n");
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
      error_msg("[EE] ext_cc1[%" PRIu32 "] not equal with easy sieving!\n", e);
      ret = -1;;
    }

    if (!ary_eql(ext_ptr_cc2, ext_ptr_cc2_cpy, sieve_words / 2, sieve_words)) {
      error_msg("[EE] ext_cc2[%" PRIu32 "] not equal with easy sieving!\n", e);
      ret = -1;;
    }

    if (!ary_eql(ext_ptr_twn, ext_ptr_twn_cpy, sieve_words / 2, sieve_words)) {
      error_msg("[EE] ext_twn[%" PRIu32 "] not equal with easy sieving!\n", e);
      ret = -1;;
    }
  }


  free(cc1_cpy);
  free(cc2_cpy);
  free(twn_cpy);
  free(ext_cc1_cpy);
  free(ext_cc2_cpy);
  free(ext_twn_cpy);

  free(cc1_layer);
  free(cc2_layer);

  if (ret == 0)
    error_msg("[DD] Succesfully tested sieveing\n");

  return ret;
}
#endif 

#ifdef CHECK_PRIMES
/**
 * checks the precalculated primes and inverses of two
 */
char check_primes(const uint32_t *const primes,
                  const uint32_t *const two_inverse, 
                  const uint32_t len) {


  if (primes[0] != 2) {
    error_msg("[EE] primes[0] != 2\n");
    return -1;
  }

  uint32_t i;
  for (i = 1; i < len; i++) {
    
    uint32_t max = sqrt(primes[i]) + 1;
    for (; max > 1; max--) {
      
      if (primes[i] % max == 0) {
        error_msg("[EE] primes[%" PRIu32 "] = %" PRIu32 "is not a prime!!!\n",
                  i,
                  primes[i]);
        return -1;
      }
    }

    if (((uint64_t) 2) * ((uint64_t) two_inverse[i]) % primes[i] != 1) {
        error_msg("[EE] two_inverse[%" PRIu32 "] = %" PRIu32 
                  "is not a valid inverse for prime %" PRIu32 "!!!\n",
                  i,
                  two_inverse[i],
                  primes[i]);

    }
  }

  error_msg("[DD] succesfully checkt primes\n");
  return 0;
}
#endif

#ifdef CHACK_SHARE

/**
 * chacks if a BlockHeader to submit is valid
 */
char check_share(BlockHeader *share, uint32_t orig_difficulty, char type) {

  mpz_t mpz_multiplier, mpz_origin, mpz_hash;

  mpz_init(mpz_multiplier);
  mpz_init(mpz_origin);
  mpz_init(mpz_hash);

  TestParams params;
  init_test_params(&params);
  
  uint8_t hash[SHA256_DIGEST_LENGTH];
  get_header_hash(share, hash);
  mpz_set_sha256(mpz_hash, hash);

  mpz_import(mpz_multiplier, 
             share->multiplier_length, 
             -1, 
             sizeof(share->primemultiplier[0]), 
             -1, 
             0, 
             share->primemultiplier);

  mpz_mul(mpz_origin, mpz_hash, mpz_multiplier);

  uint8_t chain_length = 0;
 
  if (type == BI_TWIN_CHAIN) {
    
    chain_length = twn_chain_test(mpz_origin, &params);
  } else if (type == FIRST_CUNNINGHAM_CHAIN) {

    chain_length = cc1_chain_test(mpz_origin, &params);
  } else {

    chain_length = cc2_chain_test(mpz_origin, &params);
  }

  uint32_t difficulty = chain_length << FRACTIONAL_BITS;
  difficulty += get_fractional_length(mpz_origin,
                                      type,
                                      chain_length,
                                      &params);

  if (difficulty != orig_difficulty) {
    
    error_msg("[EE] share check failed: difficulty %x orig: %x\n", 
              difficulty,
              orig_difficulty);

    return -1;
  }

  error_msg("[DD] Succesfully check share\n");
  return 0;
}

#endif

#endif /* DEBUG */
