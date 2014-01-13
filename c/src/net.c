/**
 * implementation of the getwork protocoll for poolmining
 */
#ifndef __NET_H__
#define __NET_H__

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <appa/inet.h>

#include "utils.h"

/**
 * start an keep alive tcp connection
 * return value:
 *  on succes: socket filedescriptor
 *
 * exits programm with error mesage on failur
 */
int create_socket(const char *ip; const uint16_t port) {
  
  int sock_fd = socket(AF_INET, SOCK_STREAM, 0);

  /* check for errors */
  if (sock_fd == -1) 
    perror("failed to create tcp socket");

  /* set keep alive option */
  int optval = 1;
  if (setsockopt(sock_fd, 
                 SOL_SOCKET, 
                 SO_KEEPALIVE, 
                 &optval, 
                 sizeof(optval)) == -1) {

    perror("failed to set keepalive on tcp socket");
  }

  return sock_fd;
}

/**
 * (re)connect given socket filedescriptor to the given ip and port
 * returns -1 on error
 */
int connect_to(int sock_fd, const char *ip, const uint16_t port) {
  

  /* set the addres to connect to */
  struct sockaddr_in addr;
  addr.sin_family      = AF_INET;
  addr.sin_port        = htons(port);
  addr.sin_addr.s_addr = inet_addr(ip);

  return connect(sock_fd, &addr, sizeof(struct sockaddr_in));
}



#endif /* __NET_H__ */
