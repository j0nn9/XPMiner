/**
 * Impelmentaion of the primecoin block header.
 *
 * (new transactions are saved into a block,
 *  but for poolmining we only need the block header)
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
#include <time.h>
#include <stdio.h>
#include <gmp.h>

#include "main.h"

/**
 * set a block header to zero
 */
void header_set_null(BlockHeader *header) {
  
  header->version    = BLOCK_HEADER_VERSION;
  header->time       = 0;
  header->difficulty = 0;
  header->nonce      = 0;

  memset(header->hash_prev_block,  0, HASH_LENGTH);
  memset(header->hash_merkle_root, 0, HASH_LENGTH);
  memset(header->primemultiplier,  0, MULTIPLIER_LENGTH);
}

/**
 * Returns the Block Header sha256 hash
 */
void get_header_hash(BlockHeader *header, 
                     uint8_t hash[SHA256_DIGEST_LENGTH]) {

  uint8_t tmp[SHA256_DIGEST_LENGTH];                                   
 
  SHA256_CTX sha256;                                                          
  SHA256_Init(&sha256);                                                       
  SHA256_Update(&sha256, &header->version, 4);                                   
  SHA256_Update(&sha256, header->hash_prev_block, HASH_LENGTH);                                   
  SHA256_Update(&sha256, header->hash_merkle_root, HASH_LENGTH);                                   
  SHA256_Update(&sha256, &header->time, 4);                                   
  SHA256_Update(&sha256, &header->difficulty, 4);                                   
  SHA256_Update(&sha256, &header->nonce, 4);                                   
  SHA256_Final(tmp, &sha256); 
 
  /* hash the result again */
  SHA256_Init(&sha256);                                                       
  SHA256_Update(&sha256, tmp, SHA256_DIGEST_LENGTH);  
  SHA256_Final(hash, &sha256);
}

/**
 * sets the time value for the given block header
 * (each thread gets a different time, so they
 *  don't use the same header hash for mining)
 */
void header_set_time(BlockHeader *header, 
                     uint32_t n_threads, 
                     uint32_t cur_thread) {

  
  header->time  = opts.time_offset;
  header->time += ((time(NULL) + n_threads) / n_threads) * n_threads;
  header->time += cur_thread;
}

/**
 * modify the block header (by increasing the nonce value) 
 * to have an hash divisibel by the first n primes
 * (where n is --nprimesinhash)
 *
 * the higher n is the more mining becomes like bitoin
 * (seraching for a specific sha256 hash)
 * on the other side a highly composite hash improoves
 * seraching for prime chains
 */
void mine_header_hash(Sieve *sieve, uint32_t n_threads) {

  uint8_t hash[SHA256_DIGEST_LENGTH];
  uint_fast8_t reminder = 1;

  /* mine for a hash */
  do {

    /* all nonce values used ? */
    if (sieve->header.nonce == UINT32_MAX) {

      /* addjust time */
      sieve->header.time += n_threads;
    }
    
    /* change the hash */
    sieve->header.nonce++;

    get_header_hash(&sieve->header, hash);
    
    /* skip check if hash is smaler than 2^255 */
    if (hash[SHA256_DIGEST_LENGTH - 1] & 0x80) {

      mpz_set_sha256(sieve->mpz_hash, hash);
 
      /* reminder = hash % hash_primorial */
      reminder = mpz_tdiv_ui(sieve->mpz_hash, opts.hash_primorial);
    }
  } while (running && reminder);
}
