/**
 * implementation of the default Sieve of Eratosthenes to generat 
 * a prime table including all primes till a given number starting by 2
 */
#ifndef __PRIME_TABLE_H__
#define __PRIME_TABLE_H__

/**
 * Primtable with len primes
 */
typedef struct {
  uint32_t *ptr;
  uint32_t len;
} PrimeTable;

/**
 * 
 */
PrimeTable *gen_prima_table(uint32_t sieve_size);

/**
 * creat an so called primorial form the first n primes
 * where primorial is 2 * 3 * 5 * ... 
 */
void primorial(PrimeTable *primes, mpz_t mpz_primorial, uint32_t n);

#endif /* __PRIME_TABLE_H__ */
