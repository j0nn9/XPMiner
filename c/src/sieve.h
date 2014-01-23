/**
 * Header of the Sieve of Eratosthenes for finding prime chains
 */
#ifndef __SIEVE_H__
#define __SIEVE_H__

#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <gmp.h>

#include "main.h"

/**
 * 32 or 64bit version 
 */
#ifdef USE_32_BIT_WORDS

/**
 * the sieve word type and bit length
 */
#define sieve_t uint32_t
#define word_len 32
#define word_max 0xFFFFFFFFu

#else

/**
 * the sieve word type and bit length
 */
#define sieve_t uint64_t
#define word_len 64
#define word_max 0xFFFFFFFFFFFFFFFFLu

#endif


/**
 * stucture to store sieve statistics
 */
struct SieveStats {
  uint64_t twn[MAX_CHAIN_LENGTH];
  uint64_t cc2[MAX_CHAIN_LENGTH];
  uint64_t cc1[MAX_CHAIN_LENGTH];
  uint64_t tests;
  uint64_t start_time;
};

/**
 * the sieve
 */
struct Sieve {

  sieve_t *cc1; /* bit vektor for the chain candidates of the first kind */
  sieve_t *cc2; /* bit vektor for the chain candidates of the second kind */
  sieve_t *twn; /* bit vektor for the bi-twin chain  candidates */
  sieve_t *final; /* final set fo candiates */

  /** 
   * the cc1 and cc2 multiplicators (for each layer) 
   * to sieve out the composite cc1 and cc2 members
   */
  uint32_t *cc1_muls; 
  uint32_t *cc2_muls; 

  uint32_t chain_length; /* target chain length */

  /* the byte length of the candidate bit vektor */
  uint64_t candidate_bytes;

  /* the sieve length in sieve words */
  uint64_t size;

  /* the number of sieve extensions TODO explain */
  uint32_t extensions;

  /* the number of sieve layers TODO explain */
  uint32_t layers;

  /* the higes index in the prim table to sieve */
  uint32_t max_prime_index;

  /**
   * the lowes index to start sieveing the primes 
   *
   * (this can be n_primes_in_primorial + 1
   *  because nothing in the sieve is divisible by any
   *  prime used in the primorial)
   */
  uint32_t min_prime_index;

  /* the prime table for sieveing */
  PrimeTable *primes;

  /* the nuber of bytes to load in cache */
  uint32_t cache_bytes;

  /* prime test parameters */
  TestParams test_params;

  /* reminder for mining the header hash */
  mpz_t mpz_reminder;

  /* the block header hash as an mpz_t value for integer calculations */
  mpz_t mpz_hash;

  /* the primorial trought which the header hash should be divisible */
  mpz_t mpz_hash_primorial;            

  /* mpz value for testing an prime origin */
  mpz_t mpz_test_origin;

  /* the primorial */
  mpz_t mpz_primorial;

  /**
   * the additional prim multiplyers usd in primorial 
   * (hash = primorial / primorial_multiplyers)
   */
  mpz_t mpz_primorial_primes;

  /**
   * the block header we ar mineing for
   */
  BlockHeader header;

  /**
   * the mpz_multiplier for the block hash
   * prime origin = multiplyer * hash
   */
  mpz_t mpz_multiplier;

  /**
   * reference to the options
   */
  Opts *opts;

  /**
   * indicates wether th sieve soud continue running
   */
  char active;

  /**
   * some statistcis 
   */
  SieveStats stats;


};


/** 
 * reinitilaizes an given sieve
 */
void reinit_sieve(Sieve *sieve, const Opts *opts);

/**
 * initializes a given sieve for the first
 */
void init_sieve(Sieve *sieve, Opts *const opts);

/**
 * run the sieve
 * primorial is x * 2 * 3 * 5 * 11 * 17 * ...
 *
 * where x is hash / (2 * 3 * 5 * 7)
 *
 * the bitverktor of the sieves H, 2H, 3H, 4H, ... ,nH
 * where H is the primorial
 */
void sieve_run(Sieve *sieve, const mpz_t mpz_primorial);

#endif /* __SIEVE_H__ */
