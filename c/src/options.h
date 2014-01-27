/**
 * to store program wider parameters and
 * parse comandline options
 */
#ifndef __OPTIONS_H__
#define __OPTIONS_H__

#include <inttypes.h>
#include <gmp.h>

#include "main.h"

/**
 * stucture for storing overall miner statistics
 */
struct  MiningStats {
  uint64_t share;
  uint64_t rejected;
  uint64_t stale;
  uint64_t block;
};

/**
 * Structure for the options
 */
struct Opts {
   
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
  /* number of primes the block header hash should be divisible by */
  int  n_primes_in_hash;       

  /* num of bytes to preocess in cache (while sieveing) */
  int cachebits;

  /* print extended stats */
  char verbose;

  /* output stats every n seconds */
  uint32_t stats_interval;

  /* min chainlength to submit to the pool */
  int poolshare;
  
  /**
   * number of primes the primorial should be divisible by 
   * primorial := hash * pn * pn+1 * ... * pn+k
   *
   * where pn is the first prime the has is not divisible by
   * so    n   = n_primes_in_hash + 1
   * and   n+k = n_primes_in_primorial
   */
  int  n_primes_in_primorial; 

  /**
   * the target chain length to mine
   */
  int chain_length;

  /**
   * the highes index in the primtable to sieve
   */
  int max_prime_index;
 
  /* program wide parameters */
 
  /* the primorial trought which the header hash should be divisible */
  mpz_t mpz_hash_primorial;            

  /* the prime table with the first n primes */
  PrimeTable *primes;

  /**
   * the mpz primorial
   * the sieve indexes ar factors of this
   * let H := primorial, and n := sievesize
   * then the sieve consists out of:
   * 1. layer 1H, 2H, 3H, 4H, 5H, 6H, ... ,nH
   * 2. layer 1H, 3H, 5H, 7H, 9H, 11H, ..., 2nH
   * 3. layer 1H, 5H, 9H, 13H, 17H, 21H ..., 4nH
   *  
   *  and so on, layer i has steps of 2^i till 2^i * n * H
   */
  mpz_t mpz_primorial;

  /**
   * the additional prim multiplyers usd in primorial 
   * (hash = primorial / primorial_multiplyers)
   */
  mpz_t mpz_primorial_primes;

  /**
   * the block header we are mining for
   */
  BlockHeader *header;

  /**
   * Mining statsisticts
   */
  MiningStats stats;

  /**
   * number of sieve words, since sieve size is the bit size
   */
  uint32_t sieve_words;

};

/**
 * read the comand line options into a Opts struct
 */
Opts *init_opts(int argc, char *argv[]);

/**
 * free promgram wide parameters on shutdown
 */
void free_opts(Opts *opts);

#endif /* __OPTIONS_H__ */
