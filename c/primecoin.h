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
 * fermat test pseudo prime test
 */
static inline char is_fermat_prime(mpz_t mpz_p) {
  
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
static inline char is_euler_langrange_lifchitz_prime(mpz_t mpz_n, char sophie_germain) {

  /* mpz_p = mpz_n - 1 */
  mpz_sub_ui(mpz_p, mpz_n, 1);

  /* mpz_p = mpz_p / 2 (calculated p from n) */
  mpz_tdiv_q_2exp(mpz_p, mpz_p, 1);

  /* res = (2^mpz_p) % n */
  mpz_powm(mpz_res, mpz_two, mpz_p, n);

  unsigned int n_mod8 = mpz_get_ui(n) % 8;
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
