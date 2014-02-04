/**
 * Header of the Sieve of Eratosthenes for finding prime chains
 */
#ifndef __SIEVE_H__
#define __SIEVE_H__

#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
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
#define word_bits 32
#define word_max 0xFFFFFFFFu
/* index / word_bits */
#define word_index(index) ((index) >> 5)
/* index % word_bits */
#define bit_index(index) ((index) & 0x1F)
#define PRISIEVET PRIu32

#else

/**
 * the sieve word type and bit length
 */
#define sieve_t uint64_t
#define word_bits 64
#define word_max 0xFFFFFFFFFFFFFFFFLu
/* index / word_bits */
#define word_index(index) ((index) >> 6)
/* index % word_bits */
#define bit_index(index) ((index) & 0x3F)
#define PRISIEVET PRIu64

#endif

#define byte_index(index) ((index) >> 3)

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
  sieve_t *all; /* final set fo candiates */
  sieve_t *ext_cc1; /* extendet cc1 candidates */
  sieve_t *ext_cc2; /* extendet cc2 candidates */
  sieve_t *ext_twn; /* extendet twn candidates */
  sieve_t *ext_all; /* final set of extendet candidates */

  sieve_t *cc1_layer;
  sieve_t *cc2_layer;

  /** 
   * the cc1 and cc2 multiplicators (for each layer) 
   * to sieve out the composite cc1 and cc2 members
   */
  uint32_t *cc1_muls; 
  uint32_t *cc2_muls; 

  /* prime test parameters */
  TestParams test_params;

  /* reminder for mining the header hash */
  mpz_t mpz_reminder;

  /* tmp value */
  mpz_t mpz_tmp;

  /* the block header hash as an mpz_t value for integer calculations */
  mpz_t mpz_hash;

  /* mpz value for testing an prime origin */
  mpz_t mpz_test_origin;

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
   * indicates wether th sieve soud continue running
   */
  char active;

  /**
   * some statistcis 
   */
  SieveStats stats;
};

/**
 * initializes the sieve global variables
 */
void init_sieve_globals();

/**
 * frees sieve globals
 */
void free_sieve_globals();

/**
 * sets a new header 
 */
void sieve_set_header(Sieve *sieve, BlockHeader *header);

/** 
 * reinitilaizes an given sieve
 */
void reinit_sieve(Sieve *sieve);

/**
 * initializes a given sieve for the first
 */
void init_sieve(Sieve *sieve);

/**
 * frees all used resauces of the sieve
 */
void free_sieve(Sieve *sieve);

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
