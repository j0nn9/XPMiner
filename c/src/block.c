/**
 * impelmentaion of the primecoin block header
 *
 * (new transactions are saved into a block,
 *  but for poolmining we only need the block header)
 */
#include <stdio.h>
#include <gmp.h>

#include "main.h"

/**
 * set a block header to zero
 */
void header_set_null(BlockHeader *header) {
  
  header->version = BLOCK_HEADER_VERSION;
  header->time    = 0;
  header->min_difficulty    = 0;
  header->nonce   = 0;

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
  SHA256_Update(&sha256, &header->min_difficulty, 4);                                   
  SHA256_Update(&sha256, &header->nonce, 4);                                   
  SHA256_Final(tmp, &sha256); 
 
  /* hash the result again */
  SHA256_Init(&sha256);                                                       
  SHA256_Update(&sha256, tmp, SHA256_DIGEST_LENGTH);  
  SHA256_Final(hash, &sha256);
}

/**
 * converts the given data to a BlockHeader
 */
void convert_data_to_header(uint8_t data[BLOCK_HEADER_LENGTH], 
                            BlockHeader *header) {

  header->version = *((uint32_t *) data);

  memcpy(header->hash_prev_block,  data + 4, HASH_LENGTH);
  memcpy(header->hash_merkle_root, data + 4 + HASH_LENGTH, HASH_LENGTH);

  header->time           = *((uint32_t *) (data + 4  + HASH_LENGTH * 2));
  header->min_difficulty = *((uint32_t *) (data + 8  + HASH_LENGTH * 2));
  header->nonce          = *((uint32_t *) (data + 12 + HASH_LENGTH * 2));

  memset(header->primemultiplier, 0, MULTIPLIER_LENGTH);
}

/**
 * modifoy the block header (by increasing the nonce value) 
 * to have an hash divisibel by the first n primes
 * (where n is --nprimesinhash)
 *
 * the higher n is the more mining becomes like bitoin
 * (seraching for a specific sha256 hash)
 */
void mine_header_hash(Sieve *sieve, uint32_t n_threads) {

  uint8_t hash[SHA256_DIGEST_LENGTH];

  /**
   * mine for a heash
   */
  do {
    
    /* change the hash (make sure each thread has a different hash) */
    sieve->header.nonce += n_threads;
    
    get_header_hash(&sieve->header, hash);
    mpz_set_sha256(sieve->mpz_hash, hash);

    /* reminder = hash % hash_primorial */
    mpz_mod(sieve->mpz_reminder, sieve->mpz_hash, sieve->mpz_hash_primorial);
    
  } while (running && mpz_cmp_ui(sieve->mpz_reminder, 0));
}
