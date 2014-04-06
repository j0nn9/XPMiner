/**
 * impelmentaion of the primecoin block header
 *
 * (new transactions are saved into a block,
 *  but for poolmining we only need the block header)
 */
#ifndef __BLOCK_H__
#define __BLOCK_H__

#include <inttypes.h>
#include <string.h>
#include <openssl/sha.h>
#include <stdio.h>
#include <gmp.h>

#include "main.h"

/**
 * The current header version
 * (number form original client)
 */
#define BLOCK_HEADER_VERSION 2

/**
 * the byte length of an hash
 */
#define HASH_LENGTH 32

/**
 * the byte length of the prime multiplier
 */
#define MULTIPLIER_LENGTH 47

/**
 * the block header length
 */
#define BLOCK_HEADER_LENGTH 128

/**
 * the block header
 * (using packed so gcc don't optimizes the size)
 */
struct BlockHeader {
  int32_t       version;                             
  uint8_t       hash_prev_block[HASH_LENGTH];       
  uint8_t       hash_merkle_root[HASH_LENGTH];     
  uint32_t      time;                             
  uint32_t      difficulty;                  

  /* value just to chainge the header hash */
  uint32_t      nonce;                               

  uint8_t       multiplier_length;
  uint8_t       primemultiplier[MULTIPLIER_LENGTH]; 
} __attribute__ ((packed)); 

/**
 * set a block header to zero
 */
void header_set_null(BlockHeader *header);

/**
 * Returns the Block Header sha256 hash
 */
void get_header_hash(BlockHeader *header, uint8_t hash[SHA256_DIGEST_LENGTH]);

/**
 * converts an sha256 hast to an mpz value
 * 
 * read HASH_LENGTH elements
 * last significant word first
 * last signifikat byte first
 */
#define mpz_set_sha256(mpz_res, hash) \
  mpz_import(mpz_res, HASH_LENGTH, -1, sizeof(hash[0]), -1, 0, hash)

/**
 * converts an mpz value to an 48 byte array
 */
#define mpz_to_ary(mpz_origin, ary, len) \
  mpz_export(ary, len, -1, sizeof(ary[0]), -1, 0, mpz_origin)


/**
 * sets the time value for the given block header
 */
void header_set_time(BlockHeader *header, 
                     uint32_t n_threads, 
                     uint32_t cur_thread);

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
void mine_header_hash(Sieve *sieve, uint32_t n_threads);
 


#endif /* __BLOCK_H__ */
