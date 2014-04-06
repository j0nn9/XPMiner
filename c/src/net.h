/**
 * implementation of the getwork protocoll for poolmining
 */
#ifndef __NET_H__
#define __NET_H__

#include "main.h"

/**
 * the Message types we can receive from the server
 */
#define WORK_MSG 0
#define SHARE_INFO_MSG 1


/**
 * (re)connect to the given (pool)ip and (pool)port
 */
void connect_to_pool();

/**
 * receive work from server
 * returns
 *  the message type or
 *  -1 on failure 
 */
int recv_work(MinerArgs *args);

/**
 * sends a valid share (block header) to the server
 */
void submit_share(BlockHeader *share, 
                  char type, 
                  uint32_t difficulty);

#endif /* __NET_H__ */
