/**
 * implementation of the default Sieve of Eratosthenes to generat 
 * a prime table including all primes till a given number starting by 2
 */
#ifndef __PRIME_TABLE_H__
#define __PRIME_TABLE_H__

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <inttypes.h>
#include <gmp.h>

#include "prime-table.h"

/**
 * Sets the given bit-possition in an byte array
 */
#define set_bit(ary, i) (ary[(i) / 8] |= (1 << ((i) % 8)))
    
/**
 * Unsets the given bit-possition in an byte array
 */
#define unset_bit(ary, i) (ary[(i) / 8] &= ~(1 << ((i) % 8)))

/**
 * returns wether the given bit-possition is an byte array is setted or not
 */
#define bit_at(ary, i) (ary[(i) / 8] & (1 << ((i) % 8)))

/**
 * returns wether the given index is a prime or not
 */
#define is_prime(ary, i) !bit_at(ary, i)

/**
 * generate the X**2
 */
#define POW(X) ((X) * (X))

/**
 * Primtable with len primes
 */
typedef struct {
  uint32_t *ptr;
  uint32_t len;
} PrimeTable;

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
  for (i = 6; i < sieve_size; i += 6 ) {

    i--; /* check 6n - 1*/
    if (is_prime(ary, i))
      n++;

    i += 2; /* check 6n + 1 */
    if (is_prime(ary, i))
      n++;
    
    i--; /* reset to 6n */
  }

  return n;
}

/**
 * saves the primes and frees the sieve
 */
static PrimeTable *save_primes(uint8_t *ary, uint32_t sieve_size) {
  
  PrimeTable *table = malloc(sizeof(PrimeTable));

  if (table == NULL)
    perror("failed to alloc space for the prime table");

  table->len = count_sieve(ary, sieve_size);
  table->ptr = malloc(sizeof(uint32_t) * table->len);

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
PrimeTable *gen_prima_table(uint32_t sieve_size) {

  /* bit array for sieveing */
  uint8_t *ary = calloc(sizeof(uint8_t), sieve_size / 8);

  if (ary == NULL) 
    perror("failed to allocate space for prime table generation");
  
  uint32_t i, p, limit = (uint32_t) sqrt((double) sieve_size);

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




#endif /* __PRIME_TABLE_H__ */
