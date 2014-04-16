/**
 * Implementation of the getwork protocol for pool mining.
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

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <pthread.h>
#include <math.h>

#include "main.h"

/**
 * seconds to wait until trying to reconnect
 */
#define RECONNECT_TIME 15

/**
 * mutex to avoid mutual exclusion by submitting shares
 */
static pthread_mutex_t connect_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t send_mutex    = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t fifo_mutex    = PTHREAD_MUTEX_INITIALIZER;

/**
 * the tcp socket
 */
static int tcp_socket = 0;

/**
 * length of the hello message
 */
static int hello_len = 0;

/**
 * a fifo element to store a
 * pending share
 */
typedef struct FifoE FifoE;
struct FifoE {
  char str[64];
  FifoE *next;
};

/**
 * fifo queue for pending shares
 */
typedef struct {
  FifoE *start;
  FifoE *end;
} ShareFiFo;

/**
 * the pending shares fifo queue
 */
static ShareFiFo pending_shares;

/**
 * add a new element (share) to a fifo queue
 */
static inline void fifo_add(ShareFiFo *fifo, FifoE *e) {

  pthread_mutex_lock(&fifo_mutex);   
  if (fifo->start == NULL)       
    fifo->start = e;             
                                
  if (fifo->end != NULL)         
    fifo->end->next = e;         
                                
  fifo->end = e;                 
  pthread_mutex_unlock(&fifo_mutex); 
                                
} 

/**
 * remove a element (pending share) from a fifo queue
 */
static inline FifoE *fifo_rem(ShareFiFo *fifo) {

  pthread_mutex_lock(&fifo_mutex);         
  FifoE *e = fifo->start;                     
                                      
  if (fifo->start == fifo->end)         
    fifo->end = NULL;                  
                                      
  if (fifo->start != NULL)             
    fifo->start = fifo->start->next;    
  pthread_mutex_unlock(&fifo_mutex);       
  
  return e;
} 

/**
 * (re)start an keep alive tcp connection
 */
static void create_socket() {
  
  int ret = -1;

  do {
    if (running && tcp_socket > 0) {
      close(tcp_socket);
      tcp_socket = -1;
    }

    if (running) {
      
      tcp_socket = socket(AF_INET, SOCK_STREAM, 0); 
      
      /* socket successfully created ? */
      if (running && tcp_socket >= 0) {
      
        /* set keep alive option */
        int optval = 1;
        ret = setsockopt(tcp_socket,                                                 
                         SOL_SOCKET,                                                 
                         SO_KEEPALIVE,                                               
                         &optval,                                                    
                         sizeof(int));
      } 
      
      /* recreate on error */
      if (running && (tcp_socket < 0 || ret < 0)) {
      
        errno_msg("failed to create tcp socket");
        info_msg("retrying after %ds...\n", RECONNECT_TIME);
        sleep(RECONNECT_TIME);
      }
    }
  } while (running && (tcp_socket < 0 || ret < 0));
}

/**
 * send hello message to other server
 * (the message format comes from xolominer)
 */
static int send_hello() {

  uint8_t pool_user_len = strlen(opts.pool_user);
  uint8_t pool_pwd_len  = strlen(opts.pool_pwd);
  hello_len             = pool_user_len + 23 + pool_pwd_len;
  
  uint8_t *hello = calloc(sizeof(uint8_t), hello_len);

  memcpy(hello + 1,                  opts.pool_user, pool_user_len);
  memcpy(hello + pool_user_len + 21, opts.pool_pwd,  pool_pwd_len);

  hello[0]                 = pool_user_len;
  hello[pool_user_len + 1] = 0;
  hello[pool_user_len + 2] = VERSION_MAJOR;
  hello[pool_user_len + 3] = VERSION_MINOR;
  hello[pool_user_len + 4] = opts.num_threads;
  hello[pool_user_len + 5] = opts.pool_fee;

  *((uint16_t *) (hello + pool_user_len + 6))  = opts.miner_id;
  *((uint32_t *) (hello + pool_user_len + 8))  = opts.sieve_extensions;
  *((uint32_t *) (hello + pool_user_len + 12)) = opts.sieve_percentage;
  *((uint32_t *) (hello + pool_user_len + 16)) = opts.sieve_size;

  hello[pool_user_len + 20] = pool_pwd_len;

  *((uint16_t *) (hello + pool_user_len + 21 + pool_pwd_len)) = 0;

  int ret = send(tcp_socket, hello, hello_len, MSG_DONTWAIT);

  free(hello);
  return ret;
}

/**
 * (re)connect to a given (pool)ip and (pool)port
 */
void connect_to_pool() {

  /* only one thread are allowed to (re)connect */
  pthread_mutex_lock(&connect_mutex);

  if (running) {
    
    int ret        = -1;
    int send_bytes = -1;

    do {
      /* (re)create socket */
      create_socket();
    
      if (running) {
   
        /* set the address to connect to */
        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(struct sockaddr_in));
        addr.sin_family      = AF_INET;
        addr.sin_port        = htons(opts.pool_port);
        addr.sin_addr.s_addr = inet_addr(opts.pool_ip);
        
        
        ret = connect(tcp_socket, 
                      (struct sockaddr *) &addr, 
                      sizeof(struct sockaddr_in));
        
        /* on success send hello */
        if (running && !ret)
          send_bytes = send_hello();
        
        /* wait and retry on failure */
        if (running && (ret || send_bytes != hello_len)) {
        
          errno_msg("failed to connect to pool");
          info_msg("retrying after %ds...\n", RECONNECT_TIME);
          sleep(RECONNECT_TIME);
        }
      }
    } while (running && (ret || send_bytes != hello_len));
  }

  pthread_mutex_unlock(&connect_mutex);
}

/**
 * receive work from server
 * returns
 *  the message type or
 *  -1 on failure 
 */
int recv_work(MinerArgs *args) {
  
  uint8_t msg_type         = -1;
  static int rejected      = 0;
  const uint32_t n_threads = args->n_threads;

  /* get the message type */
  if (recv(tcp_socket, &msg_type, 1, 0) != 1)
    return -1;

  /* read the rest of the message depending on the message type */
  switch (msg_type) {
    
    case WORK_MSG: { 

      /* look all mutexes so no sieve copies the header while receiving */
      uint32_t i;
      for (i = 0; i < n_threads; i++)
        pthread_mutex_lock(&args[i].mutex);


      if (recv(tcp_socket, 
               opts.header, 
               BLOCK_HEADER_LENGTH, 
               MSG_DONTWAIT) != BLOCK_HEADER_LENGTH) {

        if (!opts.quiet)
          error_msg("[EE] received invalid work\n");

        /* unlock the mutexes */
        for (i = 0; i < n_threads; i++)
          pthread_mutex_unlock(&args[i].mutex);
      
        return -1;
      }

      /* adjust time offset if necessary */
      uint32_t local_time = time(NULL);
      opts.time_offset    = (local_time > opts.header->time) ? 0 : 
                            opts.header->time - local_time;
                        

      /* unlock the mutexes */
      for (i = 0; i < n_threads; i++)
        pthread_mutex_unlock(&args[i].mutex);

      if (!opts.quiet)
        info_msg("Work received for Target: %02x.%x\n", 
                 chain_length(opts.header->difficulty),
                 fractional_length(opts.header->difficulty));

    } break;

    case SHARE_INFO_MSG: {
      const int buffer_size = 4;
      int32_t   buffer;

      if (recv(tcp_socket, &buffer, buffer_size, MSG_DONTWAIT) != buffer_size)
        return -1;

      /* get the next pending share */
      FifoE *next_share = fifo_rem(&pending_shares);

      /* received share result for non existing share */
      if (next_share == NULL)
        return -1;

      /* share rejected */
      if (buffer == 0) {
        opts.stats.rejected++;
        rejected++;
        info_msg("%s rejected\n", next_share->str);

      /* share stale */
      } else if (buffer < 0) {
        opts.stats.stale++;
        rejected = 0;
        info_msg("%s stale\n", next_share->str);

      /* share was a new block */  
      } else if (buffer > 100000) {
        opts.stats.block++;
        rejected = 0;
        info_msg("%s block!! (%" PRIu32 ")\n", next_share->str, buffer);

      /* share accepted */
      } else {
        opts.stats.share++;
        rejected = 0;
        info_msg("%s accepted (%" PRIu32 ")\n", next_share->str, buffer);
      }
      
      free(next_share);

      /* force reconnect after 3 continuous rejected shares */
      if (rejected == 3)
        connect_to_pool();
    } break;
  }

  return msg_type;
}

/**
 * sends a valid share (block header) to the server
 */
void submit_share(BlockHeader *share, 
                  char type, 
                  uint32_t difficulty) {

  pthread_mutex_lock(&send_mutex);

  char *chain_str = (type == BI_TWIN_CHAIN) ? "TWN" : 
                    (type == FIRST_CUNNINGHAM_CHAIN) ? "1CC" : "2CC";


  /* add share verbose message to the pending queue */
  FifoE *share_info = (FifoE *) calloc(sizeof(FifoE), 1);
  sprintf(share_info->str, "Found Chain: %s%02x.%06x =>",  
          chain_str, 
          chain_length(difficulty), 
          fractional_length(difficulty));

  fifo_add(&pending_shares, share_info);


  /* send the share to the pool */
  while (running && send(tcp_socket, 
                         (uint8_t *) share, 
                         BLOCK_HEADER_LENGTH, 
                         MSG_DONTWAIT) != BLOCK_HEADER_LENGTH) {

    errno_msg("failed to submit share!!!");
    connect_to_pool();
  }

  pthread_mutex_unlock(&send_mutex);
}
