/**
 * Header file of the inlinable prime tests.
 *
 * Copyright (C)  2014  Jonny Frey  <j0nn9.fr39@gmail.com>
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef __PRIMECOIN_H__
#define __PRIMECOIN_H__

#include <stdio.h>
#include <gmp.h>
#include <stdlib.h>
#include <inttypes.h>

#include "main.h"

/**
 * The Prime Chain types
 */
#define FIRST_CUNNINGHAM_CHAIN  '1'
#define SECOND_CUNNINGHAM_CHAIN '2'
#define BI_TWIN_CHAIN           'T'


/**
 * bit length of the fractional length part of the chain_length
 *
 * (chain_length = chain_length.fractional_length)
 * (  32 bit     =      8 bit  +  24 bit         )
 */
static const uint32_t FRACTIONAL_BITS = 24;

/**
 * bitmasks for the fractional and chain part of the chain_length
 */
static const uint32_t TARGET_FRACTIONAL_MASK = 0xFFFFFF;

/**
 * returns the chain_length for the given difficulty
 */
#define chain_length(difficulty) (difficulty >> FRACTIONAL_BITS)

/**
 * returns the fractional length for the given chain_length
 */
#define fractional_length(chain_length) \
  (chain_length & TARGET_FRACTIONAL_MASK)

/**
 * helper varibales for primality testing
 */
struct TestParams {

  /* GMP variables */
  mpz_t mpz_two;
  mpz_t mpz_e;
  mpz_t mpz_r;
  mpz_t mpz_r_plus_one;
  mpz_t mpz_cc1;
  mpz_t mpz_cc2;
  mpz_t mpz_n;

};

/**
 * initialize test params
 */
static inline void init_test_params(TestParams *const params) {
                                    
  mpz_init_set_ui(params->mpz_two, 2L);
  mpz_init(params->mpz_e);
  mpz_init(params->mpz_r);
  mpz_init(params->mpz_r_plus_one);
  mpz_init(params->mpz_cc1);
  mpz_init(params->mpz_cc2);
  mpz_init(params->mpz_n);
}

/**
* clear test params
*/
static inline void clear_test_params(TestParams *const params) {

  mpz_clear(params->mpz_two);
  mpz_clear(params->mpz_e);
  mpz_clear(params->mpz_r);
  mpz_clear(params->mpz_r_plus_one);
  mpz_clear(params->mpz_cc1);
  mpz_clear(params->mpz_cc2);
  mpz_clear(params->mpz_n);
}

/**
 * fermat pseudo prime test
 */
static inline char fermat_test(mpz_t mpz_p, TestParams *const params) {

  /* tmp = p - 1 */
  mpz_sub_ui(params->mpz_e, mpz_p, 1);

  /* res = 2^tmp mod p */
  mpz_powm(params->mpz_r, params->mpz_two, params->mpz_e, mpz_p);

  if (mpz_cmp_ui(params->mpz_r, 1) == 0)
    return 1;

  return 0;
}

/**
 * Calculates the fractional length for the given prime origin, 
 * and chain length
 */
static inline uint32_t get_fractional_length(mpz_t mpz_origin,
                                             char type,
                                             char chain_length,
                                             TestParams *const params) {


  if (type == BI_TWIN_CHAIN) {

    /* n = origin * chain_length / 2 */
    mpz_mul_2exp(params->mpz_n, mpz_origin, chain_length >> 1);

    /* twn chains with even length using the cc1 fractional part */
    if (chain_length % 2 == 0)
      mpz_sub_ui(params->mpz_n, params->mpz_n, 1); /* n = n - 1 */
    else 
      mpz_add_ui(params->mpz_n, params->mpz_n, 1); /* n = n + 1 */

  } else {
    
    /* n = origin * chain_length */
    mpz_mul_2exp(params->mpz_n, mpz_origin, chain_length);

    if (type == FIRST_CUNNINGHAM_CHAIN)
      mpz_sub_ui(params->mpz_n, params->mpz_n, 1); /* n = n - 1 */
    else 
      mpz_add_ui(params->mpz_n, params->mpz_n, 1); /* n = n + 1 */
  }

  /* we need fermat reminder */
  fermat_test(params->mpz_n, params);

  /* res = p - (2^(p - 1) mod p) */
  mpz_sub(params->mpz_e, params->mpz_n, params->mpz_r);

  /* res << FRACTIONAL_BITS */
  mpz_mul_2exp(params->mpz_r, params->mpz_e, FRACTIONAL_BITS);

  /* res = res / p */
  mpz_tdiv_q(params->mpz_e, params->mpz_r, params->mpz_n);

  uint32_t n_fractional_length = mpz_get_ui(params->mpz_e);

#ifdef DEBUG
  if (n_fractional_length >= (1u << FRACTIONAL_BITS))
    printf("[EE] FermatProbablePrimalityTest() : fractional assert");
#endif

  return n_fractional_length;
}

/**
 * Test probable primality of n = 2p +/- 1 based on Euler, 
 * Lagrange and Lifchitz
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
                                                TestParams *const params) {

  /* tmp = n - 1 */
  mpz_sub_ui(params->mpz_e, mpz_n, 1);

  /* tmp = tmp / 2 */
  mpz_tdiv_q_2exp(params->mpz_e, params->mpz_e, 1);

  /* res = 2^(n/2) % n */
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
      error_msg("[EE] %s : invalid n %% 8 = %d, %s",
                __func__,
                n_mod8, 
                (sophie_germain? "first kind" : "second kind"));
  

  return passed_test;
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
                                             TestParams *const params) {

  uint32_t chain_length = 0;

#ifndef USE_GMP_MILLER_RABIN_TEST
  /* Fermat test for p first */
  if (!fermat_test(mpz_p, params))
    return 0;
#else
  if (!mpz_probab_prime_p(mpz_p, 1))
    return 0;
#endif

  /* Euler-Lagrange-Lifchitz test for the following numbers in chain */
  mpz_set(params->mpz_n, mpz_p);
  
  /* loop untill chain end is reatched */
  for (;;) {
  
    chain_length++;

    /* n = n * 2 */
    mpz_mul_2exp(params->mpz_n, params->mpz_n, 1);

    if (sophie_germain) 
      mpz_add_ui(params->mpz_n, params->mpz_n, 1); /* n = n + 1 */
    else
      mpz_sub_ui(params->mpz_n, params->mpz_n, 1); /* n = n - 1 */

#ifndef USE_GMP_MILLER_RABIN_TEST
    if (!euler_lagrange_lifchitz_test(params->mpz_n, sophie_germain, params)) 
      break;
#else
    if (!mpz_probab_prime_p(params->mpz_n, 1))
      break;
#endif
  }
  
  return chain_length;
}

/**
 * Test probable prime chain for: origin (prime origin = prime +/- 1)
 * (bi-twin chains)
 *
 * Return value: length of the longest found chain
 */
static inline uint32_t twn_chain_test(mpz_t mpz_origin, 
                                      TestParams *const params) {

  uint32_t chain_length_1cc = 0;
  uint32_t chain_length_2cc = 0;

  /* mpz_cc1 = mpz_origin - 1 */
  mpz_sub_ui(params->mpz_cc1, mpz_origin, 1);
  chain_length_1cc = cunningham_chain_test(params->mpz_cc1, 1, params);

  if (chain_length_1cc >= 2) {

    /* mpz_cc2 = mpz_origin + 1 */
    mpz_add_ui(params->mpz_cc2, mpz_origin, 1);
    chain_length_2cc = cunningham_chain_test(params->mpz_cc2, 0, params);
    
    /**
     * Figure out BiTwin Chain length
     * BiTwin Chain allows a single prime at the end for odd length chain
     */
    if (chain_length_1cc > chain_length_2cc) 
      return chain_length_2cc + chain_length_2cc + 1;
    else 
      return chain_length_1cc + chain_length_1cc;
  }

  return 0;
}
/**
 * Test probable prime chain for: origin (prime = origin - 1)
 * (cunningham chain of the first kind)
 *
 * Return value: length of the longest found chain
 */
static inline uint32_t cc1_chain_test(mpz_t mpz_origin, 
                                      TestParams *const params) {

  /* mpz_cc1 = mpz_origin - 1 */
  mpz_sub_ui(params->mpz_cc1, mpz_origin, 1);
  return cunningham_chain_test(params->mpz_cc1, 1, params);
}

/**
 * Test probable prime chain for: origin (prime origin = prime + 1)
 * (cunningham chain of the second kind)
 *
 * Return value: length of the longest found chain
 */
static inline uint32_t cc2_chain_test(mpz_t mpz_origin, 
                                      TestParams *const params) {

  /* mpz_cc2 = mpz_origin + 1 */
  mpz_add_ui(params->mpz_cc2, mpz_origin, 1);
  return cunningham_chain_test(params->mpz_cc2, 0, params);
}

#endif
