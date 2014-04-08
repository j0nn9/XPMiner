/**
 * This is the header file for the comand line parsing
 * and program wide global variables initialization.
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
#ifndef __OPTIONS_H__
#define __OPTIONS_H__

#include <inttypes.h>
#include <gmp.h>
#include <time.h>

#include "main.h"

/**
 * stucture for storing overall miner statistics
 */
struct MiningStats {
  uint64_t share;
  uint64_t rejected;
  uint64_t stale;
  uint64_t block;
};

/**
 * Struct for storing the program wide options
 */
struct Opts {
  
  /* the fee for the pool (1-100) */
  uint8_t pool_fee;

  /* pool ipv4 address */
  char *pool_ip;

  /* pool tcp port */
  uint16_t pool_port;

  /* username */
  char *pool_user;
  
  /* password */
  char *pool_pwd;

  /* number of miner threads */
  uint8_t num_threads;

  /* miner id */
  uint16_t miner_id;

  /* number of sieve extensions */
  uint32_t sieve_extensions;

  /* number of primes to sieve */
  uint32_t sieve_primes;

  /* sieve size in bits */
  uint32_t sieve_size;

  /**
   * number of (the first n) primes the 
   * block header hash should be divisible by 
   */
  uint32_t primes_in_hash;       

  /**
   * number of primes the primorial (which is used for sieveing)
   * should be divisible by 
   * primorial := hash * pn * pn+1 * ... * pn+k
   *
   * where pn is the first prime the has is not divisible by
   * so    n   = primes_in_hash + 1
   * and   n+k = primes_in_primorial
   */
  uint32_t primes_in_primorial; 

  /* num of bits to precess in cache (while sieveing) */
  uint32_t cache_bits;

  /* print extended stats */
  char verbose;

  /*
   * indecates that the first half of extension 0 
   * should be used during sieveing
   * (see sieve.h for an detailed explanation)
   */
  char use_first_half;

  /* no output */
  char quiet;

  /* output stats every n seconds */
  uint32_t stats_interval;

  /* min chain length to submit to the pool */
  uint32_t pool_share;
  
  /**
   * the target chain length to mine
   */
  uint32_t chain_length;

  /**
   * the highes index in the prime table to sieve
   */
  uint32_t max_prime_index;
 
  /* the primorial trought which the header hash should be divisible */
  uint32_t hash_primorial;            

  /* the prime table with the first n primes */
  PrimeTable *primes;

  /**
   * array of the inverses of two for the prime table primes 
   */
  uint32_t *two_inverses;

  /**
   * estimated sieve percentage for connection withe the pool
   */
  uint32_t sieve_percentage;

  /**
   * the mpz primorial
   * the sieve indexes are factors of this
   * let H := primorial, and n := sieve_size
   * then the sieve consists out of:
   * 0. layer 0H, 1H, 2H,  3H,  4H,  5H, ...,  nH
   * 1. layer 0H, 2H, 4H,  6H,  8H, 10H, ..., 2nH
   * 2. layer 0H, 4H, 8H, 12H, 16H, 20H  ..., 4nH
   *  
   * and so on, layer i has steps of 2^i till 2^i * n * H
   * (look in sieve.h for a better explanation)
   */
  mpz_t mpz_primorial;

  /**
   * the additional prime multipliers used in the mpz_primorial 
   * (hash = mpz_primorial / mpz_fixed_hash_multiplier)
   */
  mpz_t mpz_fixed_hash_multiplier;

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

  /**
   * prime index after that we have to use 64 bit arithmetic
   */
  uint32_t int64_arithmetic;

  /**
   * time offset to the pool server
   */
  time_t time_offset;

  /**
   * time since program start
   */
  time_t start_time;
};

/**
 * read the comand line options into a Opts struct
 */
void init_opts(int argc, char *argv[]);

/**
 * free promgram wide parameters on shutdown
 */
void free_opts();

#endif /* __OPTIONS_H__ */
