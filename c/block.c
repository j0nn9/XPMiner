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

/**
 * The current header version
 * (number form originla client)
 */
#define CURRENT_VERSION 2

/**
 * the byte length of an hash
 */
#define HASH_LENGTH 32

/**
 * the byte length of the prime multiplier
 */
#define MULTIPLIER_LENGTH 48

/**
 * the block header 
 */
typedef struct {
  // comments: BYTES <index> + <length>
  int32_t       version;                              // 0+4
  uint8_t       hash_prev_block[HASH_LENGTH];         // 4+32
  uint8_t       hash_merkle_root[HASH_LENGTH];        // 36+32
  uint32_t      time;                                 // 68+4
  uint32_t      min_difficulty;                       // 72+4
  uint32_t      nonce;                                // 76+4
  uint8_t       primemultiplier[MULTIPLIER_LENGTH];   // 80+48
} BlockHeader;  // =128 bytes header (80 default + 48 primemultiplier)

/**
 * set a block header to zero
 */
static inline void block_header_set_null(BlockHeader *header) {
  
  header->version = CURRENT_VERSION;
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
static inline void get_header_hash(BlockHeader *header, 
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
 * converts an sha256 hast to an mpz value
 */
static inline void mpz_set_sha256(mpz_t mpz_res, uint8_t *hash) {
  /**
   * read HASH_LENGTH elements
   * last significant word first
   * last signifikat byte first
   */
  mpz_import(mpz_res, HASH_LENGTH, -1, sizeof(hash[0]), -1, 0, hash);
}


#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>

/**
 * testing
 */ 
static void print_hash(uint8_t *hash) {

  uint32_t i;
  for (i = 0; i < SHA256_DIGEST_LENGTH; i++) 
    printf("%02x", hash[i]);
}

int main(int argc, char *argv[]) {
  
  if (argc != 2) {
    printf("%s <file to read the header from>\n", argv[0]);
    exit(1);
  }

  BlockHeader header;

  int fd = open(argv[1], O_RDONLY);

  read(fd, &header.version, 4);
  read(fd, &header.hash_prev_block, HASH_LENGTH);
  read(fd, &header.hash_merkle_root, HASH_LENGTH);
  read(fd, &header.time, 4);
  read(fd, &header.min_difficulty, 4);
  read(fd, &header.nonce, 4);

  uint8_t head_hash[SHA256_DIGEST_LENGTH];
  get_header_hash(&header, head_hash);

  mpz_t hash1, hash2, hash3;
  mpz_init(hash1);
  mpz_init(hash2);
  mpz_init(hash3);

  mpz_set_sha256(hash1, header.hash_prev_block);
  mpz_set_sha256(hash2, header.hash_merkle_root);
  mpz_set_sha256(hash3, head_hash);


  printf("Header:\n");
  printf("  version %" PRIu32 "\n  hash_prev_block ", header.version);
  mpz_out_str(stdout, 10, hash1);
  printf("\n  hash_merkle_root ");
  mpz_out_str(stdout, 10, hash2);
  printf("\n  time %" PRIu32 "\n"
         "  min_difficulty %" PRIu32 "\n"
         "  nonce %" PRIu32 "\n",
         header.time,
         header.min_difficulty,
         header.nonce);

  printf("\nheader hash: ");
  print_hash(head_hash);
  printf("\n");

  return 0;
}

#endif /* __BLOCK_H__ */
