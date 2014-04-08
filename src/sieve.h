/**
 * Header of the Sieve of Eratosthenes for finding prime chains.
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
 * stucture to store the sieve related content
 *
 * the sieve is basically a variation of the Sieve of Eratosthenes.
 *
 * the indieces are all multiple of the mpz_primorial, which is
 * divisible by the first n primes (2, 3, 5, 7, 11, 13, 17, 19, 23,...)
 *
 * let H be the mpz_primorial,
 *
 * then we ar seraching for a number n with n * H +/- 1 is the first prime
 * of a prime chain, if H is divisible by prime p then we know for shure
 * that n * H +/- 1 can not be divisibe by this prime p, so we don't have
 * to sieve it. This is the cause we pumping H with lost of smal primes.
 *
 * now we want to sieve all primes greater than the ones we used in H
 * we know that if (n * H) % p == 1 then n * H + 1 is ddivisible by p
 * thus n * H + 1 is composite.
 * 
 * same for n * H - 1, if (n * H) % p == p - 1 then n * H - 1 is composite.
 *
 * so now we wannt to know when (n * H) % p is 1 (or p - 1) 
 * for this we calculate the modlular inverse vor H % p (called i)
 * now we know that i * H + 1 is divisible by p and we can sieve
 * all sieve etries in the form H * (i + n * p)
 * 
 * same for n * H - 1, we simple set i = p - i and sieve all entries
 * in the form H * (i + n * p)
 *
 * This is all simple modilo arithmetic, but to have a closer look
 *
 * let i the multiplicative inverse of H % p 
 * so i < p and ((H % p) * i) % p == 1 then
 *
 * (H * (i + n * p)) % p <=> 
 *
 * ((H % p) * ((i + n * p) % p)) % p <=>
 *
 * ((H % p) * (((i % p) + ((n * p) % p)) % p) % p) <=>
 *
 * ((H % p) * (((i % p) + 0) % p)) % p <=>
 *
 * ((H % p) * ((i + 0) % p)) % p <=>
 *
 * ((H % p) * i) % p) <=> 1
 *
 * so after we did the above with all primes form the precalculated
 * prime table, we have sieved all non primes form our sieve,
 * now to sieve all primes that are not the start of a prime chain
 * with size two we sieve all numbers in the form (n * 2H +/- 1)
 * lukiely the inverst doesn't have to be recalculated vor 2H,
 * we simplie precalculate the inverse of 2 (2^-1) for oure primes
 * and now (i * 2^-1) % p corresponds to the inverse of (2H % p)
 *
 * let 2^-1 be the inverse of 2 for the prime p then
 *
 * ((2 * H) * (2^-1 * i)) % p <=>
 *
 * (2 * H * 2^-1 * i) % p <=>
 *
 * ((2 * 2^-1) * (H * i) % p <=>
 *
 * (1 * (H * i)) % p <=>
 *
 * (H * i) % p <=>
 *
 * ((H % p) * i) % p <=> 1
 *
 * we continue the above step till, we sieved all numbers that are not 
 * propable candidate for a n prime chain.
 *
 * one of these steps is called an sieve layer:
 * in layer one we sieve numbers not prime
 * in layer two all wich ar not the start of a prime chain size two
 * and so on.
 * Form an abstract point of view, each sieve layer consist out of
 * multiples of 2^i * H (where i ist the layer index):
 *
 * 0. layer 0H, 1H, 2H,  3H,  4H,  5H, ...,  nH
 * 1. layer 0H, 2H, 4H,  6H,  8H, 10H, ..., 2nH
 * 2. layer 0H, 4H, 8H, 12H, 16H, 20H  ..., 4nH
 *  
 * and so on, layer i has steps of 2^i till 2^i * n * H
 *
 * in each layer all number not prime are sieved, so that when we sieve
 * n layers, we know that layer n-x only contains prime chain candidates
 * of length (x + 1)
 *
 * so now that we understod the principle of sieve layers, we can move on
 * to sieve extension:
 *
 * a sieve extension is a set of layers n layers so that each extension
 * consist out of prime chain candidates with length n
 *
 * the prime coin mining sieve consist out of several extensions
 * each containing one more layer than the previous, but, also dosent use
 * the first layer of the previous.
 *
 * for example
 * 
 * extension 0: consists out of layer 0 to n     (the basic sieve)
 * extension 1: consists out of layer 1 to n + 1
 * extension 2: consists out of layer 2 to n + 2
 * extension 3: consists out of layer 3 to n + 3
 * extension x: consists out of layer x to n + x
 *
 * Also each extension is double the size of the previouse one
 *
 * extension 0: 0H -  nH
 * extension 1: 0H - 2nH
 * extension 2: 0H - 4nH
 * extension 3: 0H - 8nH
 * extension x: 0H - 2^x * nH
 *
 * so the first half of each extension > 0 is coverd by the previous extension
 * thus we only have to calculate the second half of the extension
 * (each chain in the first half of a extension with lenght x is also
 *  a chain in the previous extension of size x + 1)
 *
 * the interesting point about the extensions is that the layers overlap, 
 * so wen only have to calculate them once and applay them to the extension
 * (we sieve with all primes in one layer, and make all non prime indices
 * (from the layer) in all extension wich contain the layer as non prime)
 *
 * so the only extension in which we have to calculate the first half is
 * extension 0 (but on default we don't do this, because we can't reuse
 * the layers, and so even if there are more prime candidates in this
 * first half of extension 0 it generaly slows down the overal speed)
 * 
 *
 * But one should keep in mind that in the higher extensions
 * the numbers are les likely to be prime, and also the time to check them
 * with the fermat test is longer, so we can not use endles extensions
 */
struct Sieve {

  sieve_t *cc1; /* bit vektor for the chain candidates of the first kind   */
  sieve_t *cc2; /* bit vektor for the chain candidates of the second kind  */
  sieve_t *twn; /* bit vektor for the bi-twin chain  candidates            */
  sieve_t *all; /* final set of candiates                                  */
  sieve_t *ext_cc1; /* extendet cc1 candidates                             */
  sieve_t *ext_cc2; /* extendet cc2 candidates                             */
  sieve_t *ext_twn; /* extendet twn candidates                             */
  sieve_t *ext_all; /* final set of extendet candidates                    */

  /* temp bit vectors for calculationg on layer */
  sieve_t *cc1_layer;
  sieve_t *cc2_layer;

  /** 
   * the cc1 and cc2 multiplicators (for each layer) 
   * to sieve out the composite cc1 and cc2 members
   * (these are the inverses of (n*H % p))
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
* this is also called the prove of work certificate
   */
  mpz_t mpz_multiplier;

  /**
   * indicates wether the sieve should continue running
   */
  char active;

  /**
   * some statistcis 
   */
  SieveStats stats;
};

/**
 * initializes the sieve global variables
 * (used by all mining threads)
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
 * initializes a given sieve for the first time
 */
void init_sieve(Sieve *sieve);

/**
 * frees all used resauces of the sieve
 */
void free_sieve(Sieve *sieve);

/**
 * run the sieve with a given primorial
 * primorial is x * 2 * 3 * 5 * 7 * 11 * 17 * ...
 *
 * where x is hash / (2 * 3 * 5 * 7 * ...)
 */
void sieve_run(Sieve *sieve, const mpz_t mpz_primorial);

#endif /* __SIEVE_H__ */
