/**
 * my own implementation of a primecoin miner
 */
#ifndef __PRIMECOIN_H__
#define __PRIMECOIN_H__

/* global helper mpz values */
static mpz_t mpz_two;
static mpz_t mpz_p_1;
static mpz_t mpz_res;
static mpz_t mpz_p;

/* bit used for the fractional length */
static const uint32_t n_fractional_bits = 24;
static const uint32_t TARGET_FRACTIONAL_MASK = (1u << n_fractional_bits) - 1;
static const uint32_t TARGET_LENGTH_MASK = ~TARGET_FRACTIONAL_MASK;

/**
 * init
 */
static inline void init_miner() {

  /* init static mpz values */
  mpz_init_set_ui(mpz_two, 2L);
  mpz_init(mpz_p_1);
  mpz_init(mpz_res);
  mpz_init(mpz_p);

}

/**
 * Calculates the fractional length of a given prime, and its modulo
 * restult form the fermat test
 */
static inline uint32_t get_fractional_length(mpz_t mpz_p, mpz_t mpz_fermat_res) {

    /* res = p - (2^(p - 1) mod p) */
    mpz_sub(mpz_res, mpz_p, mpz_fermat_res);

    /* res << n_fractional_bits */
    mpz_mul_2exp(mpz_res, mpz_res, n_fractional_bits);

    /* res = res / p */
    mpz_tdiv_q(mpz_res, mpz_res, mpz_p);

    uint32_t n_fractional_length = mpz_get_ui(mpz_res);

    if (n_fractional_length >= (1 << n_fractional_bits))
       printf("[EE] FermatProbablePrimalityTest() : fractional assert");

    return = n_fractional_length;
}

/**
 * fermat test pseudo prime test
 */
static inline char fermat_test(mpz_t mpz_p) {
  
  mpz_sub_ui(mpz_p_1, mpz_p, 1);
  mpz_powm(mpz_res, mpz_two, mpz_p_1, mpz_p);

  if (mpz_cmp_ui(mpz_res, 1L) == 0)
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
static inline char euler_lagrange_lifchitz_test(mpz_t mpz_n, char sophie_germain) {

  /* mpz_p = mpz_n - 1 */
  mpz_sub_ui(mpz_p, mpz_n, 1);

  /* mpz_p = mpz_p / 2 (calculated p from n) */
  mpz_tdiv_q_2exp(mpz_p, mpz_p, 1);

  /* res = (2^mpz_p) % n */
  mpz_powm(mpz_res, mpz_two, mpz_p, n);

  uint32_t n_mod8 = mpz_get_ui(n) % 8;
  char passed_test   = 0;

  /* Euler & Lagrange */
  if (sophie_germain && (n_mod8 == 7)) {
      
    /* mpz_res == 1 */
    passed_test = !mpz_cmp_ui(mpz_res, 1);

  /* Lifchitz */
  } else if (sophie_germain && (nMod8 == 3)) {

      /* mpz_res + 1 == n */
      mpz_add_ui(mpz_res, mpz_res, 1);
      passed_test = !mpz_cmp(mpz_res, n);
  
  } else if ((!sophie_germain) && (nMod8 == 5)) {
  
      /* mpz_res + 1 == n */
      mpz_add_ui(mpz_res, mpz_res, 1);
      passed_test = !mpz_cmp(mpz_res, n);
  
  } else if ((!sophie_germain) && (nMod8 == 1)) {

      /* mpz_res == 1 */
      passed_test = !mpz_cmp_ui(mpz_res, 1);
  } else
      printf("[EE] %s : invalid n %% 8 = %d, %s",
             __func__,
             nMod8, 
             (sophie_germain? "first kind" : "second kind"));
  
  return passed_test;
}

#endif
