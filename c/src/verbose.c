/**
 * code for threadsave verbose output
 */
#include <pthread.h>
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <gmp.h>

#include "main.h"

/**
 * mutex to avoid mutula excliution by writing output
 */
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;


/**
 * the license string
 */
#define LICENSE                                                               \
"    XPMiner is an standalone PrimeCoin poolminer                          \n"\
"                                                                          \n"\
"    Copyright (C)  2014  Jonny Frey                                       \n"\
"                                                                          \n"\
"    This program is free software: you can redistribute it and/or modify  \n"\
"    it under the terms of the GNU General Public License as published by  \n"\
"    the Free Software Foundation, either version 3 of the License, or     \n"\
"    (at your option) any later version.                                   \n"\
"                                                                          \n"\
"    This program is distributed in the hope that it will be useful,       \n"\
"    but WITHOUT ANY WARRANTY; without even the implied warranty of        \n"\
"    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         \n"\
"    GNU General Public License for more details.                          \n"\
"                                                                          \n"\
"    You should have received a copy of the GNU General Public License     \n"\
"    along with this program.  If not, see <http://www.gnu.org/licenses/>. \n"

/**
 * prints the license and exits the programm
 */
void print_license() {
  
  pthread_mutex_lock(&mutex);
  printf(LICENSE);
  pthread_mutex_unlock(&mutex);

  exit(EXIT_SUCCESS);
}


/**
 * the help string
 */
#define HELP                                                                  \
"  XPMiner  Copyright (C)  2014  Jonny Frey                                \n"\
"                                                                          \n"\
"Required Options:                                                         \n"\
"                                                                          \n"\
"  --pool-ip  [NUM]             ipv4 address of the pool                   \n"\
"                                                                          \n"\
"  --pool-port  [NUM]           pool port to connect to                    \n"\
"                                                                          \n"\
"  --pool-user  [STR]           pooluser name (this is normaly your xpm    \n"\
"                               address)                                   \n"\
"                                                                          \n"\
"Aditional Options:                                                        \n"\
"                                                                          \n"\
"  --license                    show license of this program               \n"\
"                                                                          \n"\
"  --pool-free  [NUM]           the fee in percent (1-100) not all pools   \n"\
"                               allowes to set these option, default: 3    \n"\
"                                                                          \n"\
"  --pool-pwd  [STR]            pool password (will be send sha1 encrypted)\n"\
"                                                                          \n"\
"  --num-threads  [NUM]         number of threads to use, default: 4       \n"\
"                                                                          \n"\
"  --miner-id  [NUM]            give your miner an id (0-65535)            \n"\
"                               defualt: 0                                 \n"\
"                                                                          \n"\
"  --sieve-extensions [NUM]     the number of sieve extension to use       \n"\
"                               this is the most senitive parameter for    \n"\
"                               tuning try the range around your           \n"\
"                               chain-length (10 +/- n)                    \n"\
"                               default: 9 (xolominer)                     \n"\
"                                                                          \n"\
"  --sieve-primes  [NUM]        the number of primes to sieve              \n"\
"                               also sensitive for tuning                  \n"\
"                               try 5000 - 50000, default: 30000           \n"\
"                                                                          \n"\
"  --sieve-size  [NUM]          the sieve size not so sensitive for        \n"\
"                               try 1000000 - 10000000, default: 4000000   \n"\
"                                                                          \n"\
"  --primes-in-hash  [NUM]      the number of primes the hash should be    \n"\
"                               divisible by (2,3,5,7 ...)                 \n"\
"                               the more primes you use the longer it      \n"\
"                               takes to calculate the header hash,        \n"\
"                               but sieveing will be faster                \n"\
"                               (use 0 - 9), default: 4                    \n"\
"                                                                          \n"\
"  --primes-in-primorial [NUM]  this is the number of aditional primes     \n"\
"                               multiplied to the header hash, the more    \n"\
"                               primes you multiplie the more chain        \n"\
"                               candidates you get from the sieve          \n"\
"                               but the prime test will run slower         \n"\
"                               because of the higer numbers, default: 5   \n"\
"                                                                          \n"\
"  --chain-length  [NUM]        the chain length you are mining for        \n"\
"                               the lower you set this the more smal       \n"\
"                               and les big chains you get, but            \n"\
"                               be aware of that pools usualy credit       \n"\
"                               higher chains better, so this sould        \n"\
"                               be the current difficulty in most cases    \n"\
"                               default: 10                                \n"\
"                                                                          \n"\
"  --cache-bits  [NUM]          the number bits to sieve at once           \n"\
"                               (cache optimation) default: 224000         \n"\
"                                                                          \n"\
"  --verbose                    print extra information                    \n"\
"                               (you will not need this is most cases)     \n"\
"                                                                          \n"\
"  --stats-interval  [NUM]      interval in seconds to print mining        \n"\
"                               statistics, default: 60                    \n"\
"                                                                          \n"\
"  --pool-share  [NUM]          smalest share credited by your pool        \n"\
"                               default: 7                                 \n"\
"                                                                          \n"\
"  --use-first-half             if given this flag indecates that, the     \n"\
"                               first half of extension 0 (the basic       \n"\
"                               sieve) should be use during sieveing       \n"\
"                               in this first half are the most and smales \n"\
"                               prime chain candidates located, but        \n"\
"                               the sieved layers can not be reuse in      \n"\
"                               the other extensions, so enabling this     \n"\
"                               mostly slows down mining a bit             \n"\
"                                                                          \n"\
"  --quiet                      print nothing                              \n"\
"                                                                          \n"

/**
 * print help message and exit
 */
void print_help() {

  pthread_mutex_lock(&mutex);
  printf(HELP);
  pthread_mutex_unlock(&mutex);

  exit(EXIT_SUCCESS);
}

/**
 * print an errno message 
 */
void errno_msg(char *msg) {

  pthread_mutex_lock(&mutex);
  perror(msg);
  pthread_mutex_unlock(&mutex);
}

/**
 * print an error message 
 */
void error_msg(char *format, ...) {

  pthread_mutex_lock(&mutex);

  va_list args;
  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);

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
 * prints an mpz value (debugging)
 */
void print_mpz(const mpz_t mpz) {
  
  pthread_mutex_lock(&mutex);
  mpz_out_str(stderr, 10, mpz);
  pthread_mutex_unlock(&mutex);
}

/**
 * generate and print statistics
 */ 
void print_stats(MinerArgs *stats, uint32_t n_threads) {

  static SieveStats sieve_stats_old;
  static uint64_t   primes_old  = 0;
  static char       initialized = 0;

  if (!initialized) {
    memset(&sieve_stats_old, 0, sizeof(SieveStats));
    initialized = 1;
  }


  SieveStats sieve_stats;
  memset(&sieve_stats, 0, sizeof(SieveStats));
  uint64_t primes = 0;

  time_t cur_time     = time(NULL);
  struct tm *timeinfo = localtime(&cur_time);

  char date_time[80];
  strftime(date_time, 80, "[%F %T]",timeinfo);

  
  /* collect the informationf from the different threads */
  uint32_t i, n;
  for (i = 0; i < n_threads; i++) {
    
    for (n = 0; n < MAX_CHAIN_LENGTH; n++) {
      sieve_stats.twn[n] += stats[i].sieve.stats.twn[n];
      sieve_stats.cc2[n] += stats[i].sieve.stats.cc2[n];
      sieve_stats.cc1[n] += stats[i].sieve.stats.cc1[n];

      if (n > 0)
        primes += stats[i].sieve.stats.twn[n] +
                  stats[i].sieve.stats.cc2[n] +
                  stats[i].sieve.stats.cc1[n];
    }

    sieve_stats.tests += stats[i].sieve.stats.tests;
  }

  /* calculate statistics */
  double passed_time  = (double) (cur_time - opts.start_time);
  uint32_t all_shares = opts.stats.share    +
                        opts.stats.rejected + 
                        opts.stats.stale    +
                        opts.stats.block;

  uint32_t test_per_sec = (sieve_stats.tests - sieve_stats_old.tests) /
                          opts.stats_interval;

  uint32_t primes_per_sec = (primes - primes_old) / opts.stats_interval;

  uint32_t _5_chains_per_hour = ((sieve_stats.twn[5] +
                                  sieve_stats.cc2[5] +
                                  sieve_stats.cc1[5]) -
                                 (sieve_stats_old.twn[5] +
                                  sieve_stats_old.cc2[5] +
                                  sieve_stats_old.cc1[5])) *
                                (3600 / opts.stats_interval);

  long double _5_chains =  ((sieve_stats.twn[5] +
                             sieve_stats.cc2[5] +
                             sieve_stats.cc1[5]));

  uint32_t avg_5_chs_per_hour = (uint32_t) (3600 * (_5_chains / passed_time));



  /* output satistics */
  info_msg("%s  T/s %" PRIu32 "  P/s %" PRIu32 "  5ch/h (%" PRIu32 
           " / %" PRIu32 ") ",
           date_time,
           test_per_sec, 
           primes_per_sec, 
           _5_chains_per_hour, 
           avg_5_chs_per_hour);

  /* generate and output found shares */
  for (i = opts.pool_share; i < MAX_CHAIN_LENGTH; i++) {
    
    uint32_t avg_chains = sieve_stats.twn[i] +                          
                          sieve_stats.cc2[i] +                          
                          sieve_stats.cc1[i];
    
    if (avg_chains > 0) {
      info_msg("%" PRIu32 "ch: %" PRIu32 " (%.1F%% | %.1F/h) ",
               i,
               avg_chains,
               ((double) avg_chains * 100) / ((double) all_shares),
               avg_chains * 3600 / passed_time);

    }
  }

  /* output share info */
  if (opts.stats.block > 0)
    info_msg("BK: %" PRIu32 " (%0.1f%%) ", 
             opts.stats.block, 
             100 * opts.stats.block / (double) all_shares);

  if (opts.stats.share > 0)
    info_msg("VL: %" PRIu32 " (%0.1f%%) ", 
             opts.stats.share, 
             100 * opts.stats.share / (double) all_shares);

  if (opts.stats.rejected > 0)
    info_msg("RJ: %" PRIu32 " (%0.1f%%) ", 
             opts.stats.rejected, 
             100 * opts.stats.rejected / (double) all_shares);

  if (opts.stats.stale > 0)
    info_msg("ST: %" PRIu32 " (%0.1f%%) ", 
             opts.stats.stale, 
             100 * opts.stats.stale / (double) all_shares);

  info_msg("\n");


  /* output all collected chains */
  if (opts.verbose) {
    info_msg("Tests: %d\n", sieve_stats.tests);

    info_msg("1CC: ");                          
    for (n = 1; n < MAX_CHAIN_LENGTH; n++)
      if (sieve_stats.cc1[n] > 0)
        info_msg("%d: [%d] ", n, sieve_stats.cc1[n]);
 
    info_msg("\n2CC: ");                          
    for (n = 1; n < MAX_CHAIN_LENGTH; n++)
      if (sieve_stats.cc2[n] > 0)
        info_msg("%d: [%d] ", n, sieve_stats.cc2[n]);
                                
                              
   
    info_msg("\nTWN: ");                          
    for (n = 1; n < MAX_CHAIN_LENGTH; n++)
      if (sieve_stats.twn[n] > 0)
        info_msg("%d: [%d] ", n, sieve_stats.twn[n]);
                              
    
    info_msg("\n\n");
  }
  

  /* save current statisticts for the next round */
  for (n = 0; n < MAX_CHAIN_LENGTH; n++) {
    sieve_stats_old.twn[n] = sieve_stats.twn[n];
    sieve_stats_old.cc2[n] = sieve_stats.cc2[n];
    sieve_stats_old.cc1[n] = sieve_stats.cc1[n];
  }

  sieve_stats_old.tests = sieve_stats.tests;
  primes_old = primes;
}

/**
 * print miner options if verbose is given
 */
void print_options() {

  if (!opts.verbose) return;
  if (opts.quiet) return;

  pthread_mutex_lock(&mutex);

  printf("%s started using following options:\n"
         "  pool-fee:                 %d\n"
         "  pool-ip:                  %s\n"
         "  pool-port:                %d\n"
         "  pool-user:                %s\n"
         "  pool-pwd:                 %s\n"
         "  pool-share:               %d\n"
         "  chain-length:             %d\n"
         "  num-threads:              %d\n"
         "  miner-id:                 %d\n"
         "  sieve-extensions:         %d\n"
         "  sieve-primes:             %d\n"
         "  sieve-size:               %d\n"
         "  cache-bits:               %d\n"
         "  primes-in-hash:           %d\n"
         "  primes-in-primorial:      %d\n"
         "  min-prime-index:          %d\n"
         "  max-prime-index:          %d\n"
         "  stats-interval:           %d\n"
         "  hash-primorial:           %d\n"
         "  use-first-half:           %s\n"
         "  fixed-hash-multiplier:    ",
         PROG_NAME,
         opts.pool_fee,
         opts.pool_ip,
         opts.pool_port,
         opts.pool_user,
         opts.pool_pwd,
         opts.pool_share,
         opts.chain_length,
         opts.num_threads,
         opts.miner_id,
         opts.sieve_extensions,
         opts.sieve_primes,
         opts.sieve_size,
         opts.cache_bits,
         opts.primes_in_hash,
         opts.primes_in_primorial,
         opts.primes_in_primorial,
         opts.max_prime_index,
         opts.stats_interval,
         opts.hash_primorial,
         (opts.use_first_half ? "true" : "false"));

  mpz_out_str(stdout, 10, opts.mpz_fixed_hash_multiplier);
  printf("\n\n");
      
  pthread_mutex_unlock(&mutex);
}
