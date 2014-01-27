/**
 * implementation of the getwork protocoll for poolmining
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

#include "main.h"

/**
 * seconts to wait untill trying to connect_to
 */
#define RECONNECT_TIME 15

/**
 * mutex to avoid mutula excliution by submiting shares
 */
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

/**
 * the tcp socket
 */
static int tcp_socket = 0;

/**
 * fifo element 
 */
typedef struct FifoE FifoE;
struct FifoE {
  char str[64];
  FifoE *next;
};

/**
 * fifo for pending shares
 */
typedef struct {
  FifoE *start;
  FifoE *end;
} ShareFiFo;

/**
 * the pending shares
 */
static ShareFiFo pending_shares;

#define fifo_add(fifo, e)       \
do {                            \
  pthread_mutex_lock(&mutex);   \
  if (fifo.start == NULL)       \
    fifo.start = e;             \
                                \
  if (fifo.end != NULL)         \
    fifo.end->next = e;         \
                                \
  fifo.end = e;                 \
  pthread_mutex_unlock(&mutex); \
                                \
} while (0)

#define fifo_rem(fifo, e)             \
do {                                  \
                                      \
  pthread_mutex_lock(&mutex);         \
  e = fifo.start;                     \
                                      \
  if (fifo.start == fifo.end)         \
    fifo.end = NULL;                  \
                                      \
  if (fifo.start != NULL)             \
    fifo.start = fifo.start->next;    \
  pthread_mutex_unlock(&mutex);       \
                                      \
} while (0)




/**
 * start an keep alive tcp connection
 */
static void create_socket() {

  pthread_mutex_lock(&mutex);
  
  tcp_socket = socket(AF_INET, SOCK_STREAM, 0); 

  /* check for errors */
  while (running && tcp_socket == -1) {
    
    errno_msg("failed to create tcp socket");
    info_msg("retrying after %ds...\n", RECONNECT_TIME);

    sleep(RECONNECT_TIME);
    tcp_socket = socket(AF_INET, SOCK_STREAM, 0);
  }



  /* set keep alive option */
  int optval = 1;
  while (running &&
         setsockopt(tcp_socket, 
                    SOL_SOCKET, 
                    SO_KEEPALIVE, 
                    &optval, 
                    sizeof(optval)) == -1) { 

    errno_msg("failed to set keepalive on tcp socket");
    info_msg("retrying after %ds...\n", RECONNECT_TIME);

    sleep(RECONNECT_TIME);
  }

  pthread_mutex_unlock(&mutex);
}

/**
 * send hello message to the server
 */
static void send_hello(Opts *opts) {

  uint8_t pooluser_len     = strlen(opts->pooluser);
  uint8_t poolpassword_bits = strlen(opts->poolpassword);
  int     hello_len        = pooluser_len + 23 + poolpassword_bits;
  
  uint8_t *hello = calloc(sizeof(uint8_t), hello_len);

  memcpy(hello + 1,                 opts->pooluser,     pooluser_len);
  memcpy(hello + pooluser_len + 21, opts->poolpassword, poolpassword_bits);

  hello[0]                = pooluser_len;
  hello[pooluser_len + 1] = 0;
  hello[pooluser_len + 2] = VERSION_MAJOR;
  hello[pooluser_len + 3] = VERSION_MINOR;
  hello[pooluser_len + 4] = opts->genproclimit;
  hello[pooluser_len + 5] = opts->poolfee;

  *((uint16_t *) (hello + pooluser_len + 6))  = opts->minerid;
  *((uint32_t *) (hello + pooluser_len + 8))  = opts->n_sieve_extensions;
  *((uint32_t *) (hello + pooluser_len + 12)) = opts->n_sieve_percentage;
  *((uint32_t *) (hello + pooluser_len + 16)) = opts->sievesize;

  hello[pooluser_len + 20] = poolpassword_bits;

  *((uint16_t *) (hello + pooluser_len + 21 + poolpassword_bits)) = 0;


  pthread_mutex_lock(&mutex);
  int ret_val = send(tcp_socket, hello, hello_len, 0);
  pthread_mutex_unlock(&mutex);

  /* send hello (connect_to by error) */
  while (running && ret_val == -1) {

    connect_to(opts);

    pthread_mutex_lock(&mutex);
    ret_val = send(tcp_socket, hello, hello_len, 0);
    pthread_mutex_unlock(&mutex);
  }

  free(hello);
}

/**
 * (re)connect to the given (pool)ip and (pool)port
 */
void connect_to(Opts *opts) {

  pthread_mutex_lock(&mutex);

  if (tcp_socket)
    close(tcp_socket);

  pthread_mutex_unlock(&mutex);
    
  /* creat socket */
  create_socket();

  /* set the addres to connect to */
  struct sockaddr_in addr;
  addr.sin_family      = AF_INET;
  addr.sin_port        = htons(opts->poolport);
  addr.sin_addr.s_addr = inet_addr(opts->poolip);


  pthread_mutex_lock(&mutex);

  int ret = connect(tcp_socket, 
                    (struct sockaddr *) &addr, 
                    sizeof(struct sockaddr_in));

  pthread_mutex_unlock(&mutex);


  while (running && ret == -1) {
    
    errno_msg("failed to connect to pool");
    info_msg("retrying after %ds...\n", RECONNECT_TIME);
    sleep(RECONNECT_TIME);
    
    /* socket file descriptor is invalid */
    if (errno == EBADF || errno == ENOTSOCK)
      create_socket();

    pthread_mutex_lock(&mutex);
 
    ret = connect(tcp_socket, 
                  (struct sockaddr *) &addr, 
                  sizeof(struct sockaddr_in));
 
    pthread_mutex_unlock(&mutex);
  }

  if (running)
    send_hello(opts);
}

/**
 * receive work from server
 * returns
 *  the message type or
 *  -1 on failure 
 */
int recv_work(Opts *opts) {
  
  uint8_t msg_type = -1;
  static int rejected = 0;

  /* get the message type */
  if (recv(tcp_socket, &msg_type, 1, 0) != 1)
    return -1;

  /* read the rest of the mesage depending on the mesage type */
  switch (msg_type) {
    
    case WORK_MSG: { 
      const int buffer_size = BLOCK_HEADER_LENGTH;
      uint8_t   buffer[buffer_size];

      if (recv(tcp_socket, buffer, buffer_size, 0) != buffer_size)
        return -1;

      convert_data_to_header(buffer, opts->header);

      info_msg("Work recieved for Target: %02x.%x\n", 
               chain_length(opts->header->min_difficulty),
               fractional_length(opts->header->min_difficulty));
    } break;

    case SHARE_INFO_MSG: {
      const int buffer_size = 4;
      int32_t   buffer;

      if (recv(tcp_socket, &buffer, buffer_size, 0) != buffer_size)
        return -1;

      /* get the next pending share */
      FifoE *next_share = NULL;
      fifo_rem(pending_shares, next_share);

      /* recieved share result for non existant share */
      if (next_share == NULL)
        return -1;

      /* share rejected */
      if (buffer == 0) {
        opts->stats.rejected++;
        rejected++;
        info_msg("%s rejected\n", next_share->str);

      /* share stale */
      } else if (buffer < 0) {
        opts->stats.stale++;
        rejected = 0;
        info_msg("%s stale\n", next_share->str);

      /* share was a new block */  
      } else if (buffer > 100000) {
        opts->stats.block++;
        rejected = 0;
        info_msg("%s block!! (%" PRIu32 ")\n", next_share->str, buffer);

      /* share accepted */
      } else {
        opts->stats.share++;
        rejected = 0;
        info_msg("%s accepted (%" PRIu32 ")\n", next_share->str, buffer);
      }
      
      free(next_share);

      /* force connect_to after 3 continous rejected shares */
      if (rejected == 3)
        connect_to(opts);
    } break;
  }

  return msg_type;
}

/**
 * sends a valid share (block header) to the server
 */
void submit_share(Opts *opts, 
                  BlockHeader *share, 
                  char *type, 
                  uint32_t difficulty) {

  /* add shares to the pending queue */
  FifoE *share_info = calloc(sizeof(FifoE), 1);
  sprintf(share_info->str, "Found Chain: %s%02x.%x =>",  
          type, 
          chain_length(difficulty), 
          fractional_length(difficulty));

  fifo_add(pending_shares, share_info);

  
  uint8_t msg[BLOCK_HEADER_LENGTH];

  memcpy(msg,                        &share->version,         4);
  memcpy(msg + 4,                    share->hash_prev_block,  HASH_LENGTH);
  memcpy(msg + 4  + HASH_LENGTH,     share->hash_merkle_root, HASH_LENGTH);
  memcpy(msg + 4  + HASH_LENGTH * 2, &share->time,            4);
  memcpy(msg + 8  + HASH_LENGTH * 2, &share->min_difficulty,  4);
  memcpy(msg + 12 + HASH_LENGTH * 2, &share->nonce,           4);
  memcpy(msg + 16 + HASH_LENGTH * 2, share->primemultiplier,  MULTIPLIER_LENGTH);

  /* procgress th folloing serial */
  pthread_mutex_lock(&mutex);

  while (running && send(tcp_socket, msg, BLOCK_HEADER_LENGTH, 0) != BLOCK_HEADER_LENGTH) {
    errno_msg("failed to submit share!!!\n");
    connect_to(opts);
  }

  pthread_mutex_unlock(&mutex);
}
