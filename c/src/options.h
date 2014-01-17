/**
 * to store program wider parameters and
 * parse comandline options
 */
#ifndef __OPTIONS_H__
#define __OPTIONS_H__

#include "prime-table.h"

/**
 * Structure for the options
 */
typedef struct {
   
  /* comand line options */
  int  poolfee;
  char *poolip;
  int  poolport;
  char *pooluser;
  char *poolpassword;
  int  genproclimit;
  int  minerid;
  int  n_sieve_extensions;
  int  n_sieve_percentage;
  int  sievesize;
  int  n_primes_in_hash;
 
  /* program wide parameters */
 
  /* the primorial trought which the header hash should be divisible */
  mpz_t mpz_hash_primorial;            

  /* the prime table with the first n primes */
  PrimeTable *primes;

  /* the block header hash as an mpz_t value for integer calculations */
  mpz_t mpz_hash;
} Opts;

/**
 * read the comand line options into a Opts struct
 */
Opts *init_opts(int argc, char *argv[]);

/**
 * free promgram wide parameters on shutdown
 */
void free_opts(Opts *opts);

#endif /* __OPTIONS_H__ */
