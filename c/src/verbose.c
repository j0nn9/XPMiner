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
void print_stats(MinerArgs *stats, uint32_t n_threads) {

  SieveStats sieve_stats;
  memset(&sieve_stats, 0, sizeof(SieveStats));

  uint32_t i, n;
  for (i = 0; i < n_threads; i++) {
    
    for (n = 0; n < MAX_CHAIN_LENGTH; n++) {
      sieve_stats.twn[n] += stats[i].sieve.stats.twn[n];
      sieve_stats.cc2[n] += stats[i].sieve.stats.cc2[n];
      sieve_stats.cc1[n] += stats[i].sieve.stats.cc1[n];
    }

    sieve_stats.tests += stats[i].sieve.stats.tests;
  }

  info_msg("Tests: %d ", sieve_stats.tests);

#if 0
  /* dont output 0-chains (not primes) */
  for (n = 1; n < MAX_CHAIN_LENGTH; n++) {

    if (sieve_stats.twn[n] + sieve_stats.cc2[n] + sieve_stats.cc1[n] > 0)
      info_msg("%d-CH: %d ", n, sieve_stats.twn[n] + 
                                sieve_stats.cc2[n] +
                                sieve_stats.cc1[n]);
  }
  info_msg("\n");
#endif

  /* dont output 0-chains (not primes) */
  for (n = 1; n < MAX_CHAIN_LENGTH; n++) {

    if (sieve_stats.cc1[n] > 0)
      info_msg("%d-1CC: %d ", n, sieve_stats.cc1[n]);

    if (sieve_stats.cc2[n] > 0)
      info_msg("%d-2CC: %d ", n, sieve_stats.cc2[n]);
                              
    if (sieve_stats.twn[n] > 0)
      info_msg("%d-TWN: %d ", n, sieve_stats.twn[n]);
                            
  }
  
  info_msg("\n");
}
