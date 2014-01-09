/**
 * impelmentaion of the primecoin blocks
 *
 * Nodes collect new transactions into a block, hash them into a hash tree,
 * and scan through nonce values to make the block's hash satisfy proof-of-work
 * requirements.  When they solve the proof-of-work, they broadcast the block
 * to everyone and the block is added to the block chain.  The first transaction
 * in the block is a special one that creates a new coin owned by the creator
 * of the block.
 */
#ifndef __BLOCK_H__
#define __BLOCK_H__

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
  int32_t       nVersion;                             // 0+4
  uint64_t      hashPrevBlock[HASH_LENGTH];           // 4+32
  uint64_t      hashMerkleRoot[HASH_LENGTH];          // 36+32
  uint32_t      nTime;                                // 68+4
  /* wantet minimum difficulty */
  uint32_t      nBits;                                // 72+4
  uint32_t      nNonce;                               // 76+4
  uint64_t      primemultiplier[MULTIPLIER_LENGTH];   // 80+48
} BlockHeader;  // =128 bytes header (80 default + 48 primemultiplier)

/**
 * set a block header to zero
 */
static inline void block_header_set_null(BlockHeader *header) {
  
  header->nVersion = CURRENT_VERSION;
  header->nTime    = 0;
  header->nBits    = 0;
  header->nNonce   = 0;

  memset(header->hashPrevBlock,          0, HASH_LENGTH);
  memset(header->hashMerkleRoot,         0, HASH_LENGTH);
  memset(header->bnPrimeChainMultiplier, 0, MULTIPLIER_LENGTH);
}

#endif /* __BLOCK_H__ */
