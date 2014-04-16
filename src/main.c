/**
 * Implementation of an Primecoin (XPM) Miner called XPMiner.
 * This is the main file which starts everything
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
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>

#define __USE_GNU
#include <pthread.h>
#undef __USE_GNU

#define EXTERN
#include "main.h"
#undef EXTERN

/**
 * thread to output status informations
 */
void *stats_thread(void *thread_args) {
  
  MinerArgs *stats   = (MinerArgs *) thread_args;
  uint64_t n_threads = stats[0].n_threads;

  /* wait until mining started */
  uint64_t i;
  for (i = 0; i < n_threads; i++) 
    while (running && stats[i].mine != MINING_STARTED)
      pthread_yield();

  /* loop until shutdown */
  while (running) {

    sleep(opts.stats_interval);
    print_stats(stats, n_threads);
  }

  return NULL;
}

/**
 * the actual Primecoin miner which starts the sieve and so on
 */
void *primcoin_miner(void *thread_args) {

  MinerArgs *args          = (MinerArgs *) thread_args;
  Sieve *const sieve       = &args->sieve;
  const uint32_t id        = args->id;
  const uint32_t n_threads = args->n_threads;

  /* the primorial to create the sieve form */
  mpz_t mpz_primorial;
  mpz_init(mpz_primorial);

  /* initialize the sieve */
  init_sieve(sieve);

  /* waiting to get started */
  while (running && args->mine == MINING_WAIT)
    pthread_yield();

  args->mine = MINING_STARTED;

  /* start mining */
  while (running) {

    /* reset nonce if new work arrived */
    if (args->new_work) {

      pthread_mutex_lock(&args->mutex); 

      if (opts.verbose)
        info_msg("[Thread-%" PRIu32 "] got new work\n", args->id);

      args->new_work = 0;

      /* reinit sieve */
      sieve_set_header(sieve, opts.header);

      pthread_mutex_unlock(&args->mutex);

      /* init time (server - client offset)*/
      header_set_time(&sieve->header, n_threads, id);

      /* set nonce to zero */
      sieve->header.nonce = 0;
    }

    reinit_sieve(sieve);

    /* generate a hash divisible by the hash primorial */
    mine_header_hash(sieve, args->n_threads);

    /* calculate the primorial for sieving */
    mpz_mul(mpz_primorial, sieve->mpz_hash, opts.mpz_fixed_hash_multiplier);

    /* run the sieve and check the candidates */
    sieve_run(sieve, mpz_primorial);
  }

  args->mine = MINING_STOPPED;
  mpz_clear(mpz_primorial);

  free_sieve(sieve);

  return NULL;
}

/**
 * main thread which starts the miner threads
 * and the output (stats) thread
 */
void main_thread(MinerArgs *args) {
 
  pthread_t stats;
  pthread_t *threads = malloc(opts.num_threads * sizeof(pthread_t));

  char args_given = (args != NULL);

  if (args == NULL)
    args = (MinerArgs *) malloc(opts.num_threads * sizeof(MinerArgs));

  memset(args, 0, opts.num_threads * sizeof(MinerArgs));

  int i;

  /* init thread specific part of the args */
  for (i = 0; i < opts.num_threads; i++) {
    args[i].mine      = MINING_WAIT;
    args[i].n_threads = opts.num_threads;
    args[i].id        = i;
    args[i].new_work  = 1;

    pthread_mutex_init(&args[i].mutex, NULL);
    pthread_create(&threads[i], NULL, primcoin_miner, (void *) &args[i]);
  }

  if (!opts.quiet) 
    pthread_create(&stats, NULL, stats_thread, (void *) args);

  /* connect to pool */
  connect_to_pool();

  /* loop until program gets closed */
  while (running) {
    
    /* wait for work from pool */
    switch (recv_work(args)) {
      
      case WORK_MSG: 
        for (i = 0; i < opts.num_threads; i++) {

          pthread_mutex_lock(&args[i].mutex);

          args[i].new_work = 1;
          args[i].sieve.active = 0;
          
          if (args[i].mine == MINING_WAIT)
            args[i].mine = MINING_START;

          pthread_mutex_unlock(&args[i].mutex);
        }
        break;

      case SHARE_INFO_MSG:
        /* do nothing special */
        break;

      /* reconnect on failure */
      default:
        if (running)
          connect_to_pool();

    }
  }

  /* shutdown stats thread */
  if (!opts.quiet) {
    pthread_kill(stats, SIGUSR1);
    pthread_join(stats, NULL);
  }

  /* shutdown miner threads */
  for (i = 0; i < opts.num_threads; i++) {
    args[i].new_work = 0;
    args[i].sieve.active = 0;
  }

  /* wait for threads to finish */
  for (i = 0; i < opts.num_threads; i++) 
    pthread_join(threads[i], NULL);
  
  free(threads);

  if (!args_given)
    free(args);
}

/**
 * signal handler to exit program softly
 */
void soft_shutdown(int signum) {
  
  /* stats thread killed */
  if (signum == SIGUSR1) return;

  static int shutdown = 0;
  running = 0;

  if (shutdown >= 5) {
    if (!opts.quiet)
      info_msg("\rOK i'm going to KILL myself!!!\n");

    kill(0, SIGKILL);
  }

  if (!opts.quiet)
    info_msg("\rshutting down...\n");

  shutdown++;
}

/**
 * start everything (main program start)
 */
int main(int argc, char *argv[]) {

  /**
   * indicates that the program should run
   */
  running = 1;

  /* set signal handler for soft shutdown */
  struct sigaction action;
  memset(&action, 0, sizeof(struct sigaction));

  action.sa_handler = soft_shutdown;
  sigaction(SIGINT,  &action, NULL);
  sigaction(SIGTERM, &action, NULL);
  sigaction(SIGALRM, &action, NULL);
  sigaction(SIGUSR1, &action, NULL); /* to kill stats thread */

  /* continue mining if terminal lost connection */
  action.sa_handler = SIG_IGN;
  sigaction(SIGHUP,  &action, NULL);
  sigaction(SIGPIPE, &action, NULL); 
  sigaction(SIGQUIT, &action, NULL);

  /* read the command line options, and initialize program wide parameters */
  init_opts(argc, argv);

  /* print options if --verbose was given */
  print_options();

  /* start mining */
  main_thread(NULL);

  free_opts();

  return 0;
}
