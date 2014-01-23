/**
 * code for threadsave verbose output
 */
#include <pthread.h>
#include <stdio.h>
#include <stdarg.h>

#include "main.h"

/**
 * mutex to avoid mutula excliution by writing output
 */
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

/**
 * print an error message 
 */
void errno_msg(char *msg) {

  pthread_mutex_lock(&mutex);
  
  perror(msg);

  pthread_mutex_unlock(&mutex);
}

/**
 * printf an formated string 
 */
void info_msg(char *format, ...) {
  
  pthread_mutex_lock(&mutex);

  va_list args;
  va_start(args, format);
  vprintf(format, args);
  va_end(args);

  pthread_mutex_unlock(&mutex);

}

/**
 * generate and print statistics
 */ 
void print_stats(MiningStats stats, Sieve *sieves, uint32_t n_threads) {

  (void) stats;
  (void) sieves;
  (void) n_threads;
  
  printf("sats comming soon xD\n");

}
