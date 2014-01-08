/**
 * my own implementation of a primecoin miner
 */
#ifndef __PRIMECOIN_H__
#define __PRIMECOIN_H__

#include <stdio.h>
#include <stdlib.h>
#include <gmp.h>
#include <inttypes.h>

/**
 * bit length of the fractional length part of the difficulty
 *
 * (difficulty = difficulty.fractional_length)
 * (  32 bit   =      8 bit  +  24 bit         )
 */
static const uint32_t N_FRACTIONAL_BITS = 24;

/**
 * bitmasks for the fractional and chain part of the difficulty
 */
static const uint32_t TARGET_FRACTIONAL_MASK = 16777215; //(1u << N_FRACTIONAL_BITS) - 1;
static const uint32_t TARGET_LENGTH_MASK = 4261412864; //~TARGET_FRACTIONAL_MASK;

/**
 * Prime Chain types
 */
enum {
  PRIME_CHAIN_CUNNINGHAM1 = 1u,
  PRIME_CHAIN_CUNNINGHAM2 = 2u,
  PRIME_CHAIN_BI_TWIN     = 3u,
};

/**
 * increases a given difficulty by 1
 */
#define inc_difficulty(difficulty) (difficulty) += TARGET_FRACTIONAL_MASK + 1

/**
 * adds a given chain length to the given difficulty    
 */
#define add_chain_length(difficulty, chain_length) \
  (difficulty) = (difficulty) + (TARGET_FRACTIONAL_MASK + 1) * chain_length

/**
 * adds a given fractional part to a given difficulty
 */
#define add_fractional_length(difficulty, fractional_length)              \
  (difficulty) = ((difficulty) & TARGET_LENGTH_MASK) | (fractional_length)

/**
 * returns the chain length form a given difficulty
 */
#define chain_length(difficulty) ((difficulty) >> N_FRACTIONAL_BITS)

/**
 * helper varibales for primality testing
 */
typedef struct {

  /* GMP variables */
  mpz_t mpz_two;
  mpz_t mpz_e;
  mpz_t mpz_r;
  mpz_t mpz_r_plus_one;
  mpz_t mpz_origin_minus_one;
  mpz_t mpz_origin_plus_one;
  mpz_t mpz_n;

  // Values specific to a round
  uint32_t n_bits;
  uint32_t n_primorial_seq;
  uint32_t chain_type;

  // Results
  uint32_t difficulty;

} TestParams;

/**
 * initialize test params
 */
static inline void init_test_params(TestParams *params, 
                                    uint32_t n_bits, 
                                    uint32_t n_primorial_seq) {

  params->n_bits          = n_bits;
  params->n_primorial_seq = n_primorial_seq;
  params->difficulty      = 0;

  mpz_init_set_ui(params->mpz_two, 2L);
  mpz_init(params->mpz_e);
  mpz_init(params->mpz_r);
  mpz_init(params->mpz_r_plus_one);
  mpz_init(params->mpz_origin_minus_one);
  mpz_init(params->mpz_origin_plus_one);
  mpz_init(params->mpz_n);
}

/**
 * clear test params
 */
static inline void clear_test_params(TestParams *params) {

  mpz_clear(params->mpz_two);
  mpz_clear(params->mpz_e);
  mpz_clear(params->mpz_r);
  mpz_clear(params->mpz_r_plus_one);
  mpz_clear(params->mpz_origin_minus_one);
  mpz_clear(params->mpz_origin_plus_one);
  mpz_clear(params->mpz_n);
}

/**
 * Calculates the fractional length of a given prime, and its modulo
 * restult form the fermat test
 */
static inline uint32_t get_fractional_length(mpz_t mpz_p,
                                             TestParams *params) {

  /* res = p - (2^(p - 1) mod p) */
  mpz_sub(params->mpz_e, mpz_p, params->mpz_r);

  /* res << N_FRACTIONAL_BITS */
  mpz_mul_2exp(params->mpz_r, params->mpz_e, N_FRACTIONAL_BITS);

  /* res = res / p */
  mpz_tdiv_q(params->mpz_e, params->mpz_r, mpz_p);

  uint32_t n_fractional_length = mpz_get_ui(params->mpz_e);

  if (n_fractional_length >= (1u << N_FRACTIONAL_BITS))
    printf("[EE] FermatProbablePrimalityTest() : fractional assert");

  return n_fractional_length;
}

/**
 * fermat test pseudo prime test
 */
static inline char fermat_test(mpz_t mpz_p, TestParams *params) {

  /* tmp = p - 1 */
  mpz_sub_ui(params->mpz_e, mpz_p, 1);

  /* res = 2^tmp mod p */
  mpz_powm(params->mpz_r, params->mpz_two, params->mpz_e, mpz_p);

  if (mpz_cmp_ui(params->mpz_r, 1L) == 0)
    return 1;

  return 0;
}

/**
 * Test probable primality of n = 2p +/- 1 based on Euler, Lagrange and Lifchitz
 *
 * more infos: http://www.primenumbers.net/Henri/us/NouvTh1us.htm
 *
 * sophie_germain:
 *    1: test for Cunningham Chain of the first kind:  n = 2p + 1
 *    0: test for Cunningham Chain of the second kind: n = 2p - 1
 *  returns wether the n is a prime or not
 */
static inline char euler_lagrange_lifchitz_test(mpz_t mpz_n, 
                                                char sophie_germain,
                                                TestParams *params) {

  /* tmp = n - 1 */
  mpz_sub_ui(params->mpz_e, mpz_n, 1);

  /* tmp = tmp / 2 */
  mpz_tdiv_q_2exp(params->mpz_e, params->mpz_e, 1);

  /* res = 2^p % n */
  mpz_powm(params->mpz_r, params->mpz_two, params->mpz_e, mpz_n);

  uint32_t n_mod8  = mpz_get_ui(mpz_n) % 8;
  char passed_test = 0;

  /* Euler & Lagrange */
  if (sophie_germain && (n_mod8 == 7)) {
      
    /* mpz_r == 1 */
    passed_test = !mpz_cmp_ui(params->mpz_r, 1);

  /* Lifchitz */
  } else if (sophie_germain && (n_mod8 == 3)) {

      /* mpz_r + 1 == n */
      mpz_add_ui(params->mpz_r_plus_one, params->mpz_r, 1);
      passed_test = !mpz_cmp(params->mpz_r_plus_one, mpz_n);
  
  } else if ((!sophie_germain) && (n_mod8 == 5)) {
  
      /* mpz_r + 1 == n */
      mpz_add_ui(params->mpz_r_plus_one, params->mpz_r, 1);
      passed_test = !mpz_cmp(params->mpz_r_plus_one, mpz_n);
  
  } else if ((!sophie_germain) && (n_mod8 == 1)) {

      /* mpz_r == 1 */
      passed_test = !mpz_cmp_ui(params->mpz_r, 1);
  } else
      printf("[EE] %s : invalid n %% 8 = %d, %s",
             __func__,
             n_mod8, 
             (sophie_germain? "first kind" : "second kind"));
  
  if (passed_test) 
    return 1;

  /**
   * test failed calculate fermat test result for n
   * (to calculate the fractional length form params->mpz_r)
   */

  /* tmp = res * res */
  mpz_mul(params->mpz_e, params->mpz_r, params->mpz_r);

  /* res = tmp / n */
  mpz_tdiv_r(params->mpz_r, params->mpz_e, mpz_n); 

  return 0;
}

/**
 * Test Probable Cunningham Chain for: p
 *
 * sophie_germain:
 *   true:   Test for Cunningham Chain of first kind  (p, 2p+1, 4p+3, ...)
 *   false:  Test for Cunningham Chain of second kind (p, 2p-1, 4p-3, ...)
 *
 * Return value: chain length for p
 */
static inline uint32_t cunningham_chain_test(mpz_t mpz_p, 
                                             char sophie_germain, 
                                             char use_fermat_test, 
                                             TestParams *params) {

  uint32_t difficulty = 0;

  /* Fermat test for p first */
  if (!fermat_test(mpz_p, params))
      return 0;

  /* Euler-Lagrange-Lifchitz test for the following numbers in chain */
  mpz_set(params->mpz_n, mpz_p);
  
  /* loop untill chain end is reatched */
  for (;;) {
  
    inc_difficulty(difficulty);

    /* n = n * 2 */
    mpz_mul_2exp(params->mpz_n, params->mpz_n, 1);

    if (sophie_germain) 
      mpz_add_ui(params->mpz_n, params->mpz_n, 1); /* n = n + 1 */
    else
      mpz_sub_ui(params->mpz_n, params->mpz_n, 1); /* n = n - 1 */

    if (use_fermat_test) {

      if (!fermat_test(params->mpz_n, params)) 
        break;
    } else {

      if (!euler_lagrange_lifchitz_test(params->mpz_n, sophie_germain, params)) 
        break;
    }
  }
  
  add_fractional_length(difficulty, get_fractional_length(params->mpz_n, params));

  return difficulty;
}


/**
 * Test probable prime chain for: origin (prime origin = prime +/- 1)
 *
 * Return value: length of the longest found chain
 */
static inline uint32_t probable_prime_chain_test(mpz_t mpz_origin, 
                                             TestParams *params) {

  uint32_t difficulty = 0;

  /* Test for Cunningham Chain of first kind */
  if (params->chain_type == PRIME_CHAIN_CUNNINGHAM1) {
  
    /* mpz_origin_minus_one = mpz_origin - 1 */
    mpz_sub_ui(params->mpz_origin_minus_one, mpz_origin, 1);
    difficulty = cunningham_chain_test(params->mpz_origin_minus_one, 1, 0, params);

  /* Test for Cunningham Chain of second kind */
  } else if (params->chain_type == PRIME_CHAIN_CUNNINGHAM2) {
  
    /* mpz_origin_minus_one = mpz_origin + 1 */
    mpz_add_ui(params->mpz_origin_minus_one, mpz_origin, 1);
    difficulty = cunningham_chain_test(params->mpz_origin_minus_one, 0, 0, params);
      
  /* Test for BiTwin Chains */
  } else {
  
    uint32_t difficulty_1cc = 0;
    uint32_t difficulty_2cc = 0;

    /* mpz_origin_minus_one = mpz_origin - 1 */
    mpz_sub_ui(params->mpz_origin_minus_one, mpz_origin, 1);
    difficulty_1cc = cunningham_chain_test(params->mpz_origin_minus_one, 1, 0, params);

    if (chain_length(difficulty_1cc) >= 2) {

      /* mpz_origin_minus_one = mpz_origin + 1 */
      mpz_add_ui(params->mpz_origin_minus_one, mpz_origin, 1);
      difficulty_2cc = cunningham_chain_test(params->mpz_origin_minus_one, 0, 0, params);
      
      /**
       * Figure out BiTwin Chain length
       * BiTwin Chain allows a single prime at the end for odd length chain
       */
      if (chain_length(difficulty_1cc) > chain_length(difficulty_2cc)) {
        difficulty = difficulty_2cc + (TARGET_FRACTIONAL_MASK + 1) * (chain_length(difficulty_1cc) + 1);
      } else {
        difficulty = difficulty_1cc + (TARGET_FRACTIONAL_MASK + 1) * chain_length(difficulty_2cc);
      }
    }
  }

  return difficulty;
}

#include <unistd.h>

int main() {
  
  TestParams params;
  init_test_params(&params, 0, 0);
  uint32_t difficulty_1cc;
  uint32_t difficulty_2cc;
  uint32_t difficulty_twn;

  mpz_t mpz_i;
  /* run trough all prime twin origins */
  for (mpz_init_set_ui(mpz_i, 6); /* forever */; mpz_add_ui(mpz_i, mpz_i, 6)) {

    params.chain_type = PRIME_CHAIN_CUNNINGHAM1;
    difficulty_1cc    = probable_prime_chain_test(mpz_i, &params);

    params.chain_type = PRIME_CHAIN_CUNNINGHAM2;
    difficulty_2cc    = probable_prime_chain_test(mpz_i, &params);

    params.chain_type = PRIME_CHAIN_BI_TWIN;
    difficulty_twn    = probable_prime_chain_test(mpz_i, &params);

#if 0
    if (difficulty_1cc > 0) {
      printf("[II] testing: ");
      mpz_out_str(stdout, 10, mpz_i);
      printf("\r");
      usleep(100000);
    }
#endif

    if (chain_length(difficulty_twn) >= 2) {
      printf("[TWN] len: %" PRIu32 ", origin: ", chain_length(difficulty_twn));
      mpz_out_str(stdout, 10, mpz_i);
      printf("\n");

    } else if (chain_length(difficulty_1cc) >= 2) {
      printf("[1CC] len: %" PRIu32 ", origin: ", chain_length(difficulty_1cc));
      mpz_out_str(stdout, 10, mpz_i);
      printf("\n");

    } else if (chain_length(difficulty_2cc) >= 2) {
      printf("[2CC] len: %" PRIu32 ", origin: ", chain_length(difficulty_2cc));
      mpz_out_str(stdout, 10, mpz_i);
      printf("\n");
    }
  }

  clear_test_params(&params);
  return 0;
}

#endif
