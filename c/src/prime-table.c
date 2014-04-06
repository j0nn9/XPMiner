/**
 * implementation of the default Sieve of Eratosthenes to generat 
 * a prime table including all primes till a given number starting by 2
 */
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <inttypes.h>
#include <gmp.h>

#include "main.h"

/**
 * Sets the given bit-possition in an byte array
 */
#define set_bit(ary, i) (ary[(i) >> 3] |= (1 << ((i) & 0x7)))
    
/**
 * Unsets the given bit-possition in an byte array
 */
#define unset_bit(ary, i) (ary[(i) >> 3] &= ~(1 << ((i) & 0x7)))

/**
 * returns wether the given bit-possition is an byte array is setted or not
 */
#define bit_at(ary, i) (ary[(i) >> 3] & (1 << ((i) & 0x7)))

/**
 * returns wether the given index is a prime or not
 */
#define is_prime(ary, i) !bit_at(ary, i)

/**
 * generate the X**2
 */
#define POW(X) ((X) * (X))

/**
 * returns the number of primes in the sive
 */
static uint64_t count_sieve(uint8_t *ary, uint32_t sieve_size) {

  /* 2 and 3 are not counted in the loop */
  uint64_t i, n = 2;

  /** 
   * run the sieve in seps of size 6 
   * (each prime > 3 will be on one of the folloing palces: 6n +/- 1)
   */
  for (i = 5; i < sieve_size; i += 4 ) {

    /* check 6n - 1*/
    if (is_prime(ary, i))
      n++;

    i += 2; /* check 6n + 1 */
    if (is_prime(ary, i))
      n++;
  }

  return n;
}

/**
 * saves the primes and frees the sieve
 */
static PrimeTable *save_primes(uint8_t *ary, uint32_t sieve_size) {
  
  PrimeTable *table = (PrimeTable *) malloc(sizeof(PrimeTable));

  if (table == NULL)
    errno_msg("failed to alloc space for the prime table");

  table->len = count_sieve(ary, sieve_size);
  table->ptr = (uint32_t *) malloc(sizeof(uint32_t) * table->len);

  /* 2 and 3 are not counted in the loop */
  uint64_t i, n = 2;
  table->ptr[0] = 2;
  table->ptr[1] = 3;

  /** 
   * run the sieve in seps of size 6 
   * (each prime > 3 will be on one of the folloing palces: 6n +/- 1)
   */
  for (i = 5; i < sieve_size; i += 4 ) {

    /* check 6n - 1*/
    if (is_prime(ary, i)) {
      table->ptr[n] = i;
      n++;
    }

    i += 2; /* check 6n + 1 */
    if (is_prime(ary, i)) {
      table->ptr[n] = i;
      n++;
    }
  }

  free(ary);
  return table;
}

/**
 * 
 */
PrimeTable *gen_prime_table(uint32_t sieve_size) {

  /* bit array for sieveing */
  uint8_t *ary = (uint8_t *) calloc(sizeof(uint8_t), sieve_size / 8);

  if (ary == NULL) 
    errno_msg("failed to allocate space for prime table generation");
  
  uint32_t i, p, limit = (uint32_t) (sqrt((double) sieve_size) + 1);

  /* 0 and 1 ar not primes */
  set_bit(ary, 0);
  set_bit(ary, 1);

  /* sieve 2 and 3 first */
  for (i = 2; i < 4; i++) 
    for (p = POW(i); p < sieve_size; p += i)
      set_bit(ary, p);

  /** 
   * now run the sieve in seps of size 6 
   * (each prime > 3 will be on one of the folloing palces: 6n +/- 1)
   */
  for (i = 5; i < limit; i += 4) {

    /* test 6n - 1 */
    if (is_prime(ary, i)) 
      for (p = POW(i); p < sieve_size; p += i) 
        set_bit(ary, p);

    i += 2; /* test 6n + 1 */
    if (is_prime(ary, i)) 
      for (p = POW(i); p < sieve_size; p += i) 
        set_bit(ary, p);
  }

  return save_primes(ary, sieve_size);
}

/**
 * creat an so called primorial 
 * (a composite number out of a given range of primes)
 */
void primorial(PrimeTable *primes, 
               mpz_t mpz_primorial, 
               uint32_t start, 
               uint32_t end) {

  mpz_set_ui(mpz_primorial, 1);

  uint32_t i;
  for (i = start; i < end; i++)
    mpz_mul_ui(mpz_primorial, mpz_primorial, primes->ptr[i]);
}

/**
 * creat an so called primorial 
 * (a composite number out of a given range of primes)
 */
void int_primorial(PrimeTable *primes, uint32_t *primorial, uint32_t end) {

  *primorial = 1;

  uint32_t i;
  for (i = 0; i < end; i++)
    *primorial *= primes->ptr[i];
}
