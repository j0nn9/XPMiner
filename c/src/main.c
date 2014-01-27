/**
 * Implementation of an Primecoin (XPM) Miner
 * (main this is weher it begins)
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

/**
 * thread to output statsus informations
 */
void *stats_thread(void *thread_args) {
  
  MinerArgs *stats   = (MinerArgs *) thread_args;
  Opts *opts         = stats[0].opts;
  uint64_t n_threads = stats[0].n_threads;

  /* wait untill mining started */
  uint64_t i;
  for (i = 0; i < n_threads; i++) 
    while (running && stats[i].mine != MINING_STARTED)
      pthread_yield();

  /* loop until shutdown */
  while (running) {

    sleep(opts->stats_interval);
    print_stats(stats, n_threads);
  }

  return NULL;
}

/**
 * the actual primecoin miner which starts the sieve and so on
 */
void *primcoin_miner(void *thread_args) {

  MinerArgs *args = (MinerArgs *) thread_args;
  Sieve *const sieve = &args->sieve;
  Opts *const opts = args->opts;

  /* the primorial to create the sieve form */
  mpz_t mpz_primorial;
  mpz_init(mpz_primorial);

  /* initilaize sieve */
  init_sieve(sieve, opts);

  /* waiting to get started */
  while (running && args->mine == MINING_WAIT)
    pthread_yield();

  args->mine = MINING_STARTED;

  /* start mining */
  while (running) {

    /* reset nonce if new work arrieved */
    if (args->new_work) {

      pthread_mutex_lock(&args->mutex); 

      args->new_work = 0;

      /* reinit sieve */
      sieve_set_header(sieve, opts->header);

      sieve->header.nonce = args->id;

      pthread_mutex_unlock(&args->mutex);
    } 

    /* reinit sieve */
    reinit_sieve(sieve);

    /* generate a hash divisible by the hash primorial */
    mine_header_hash(sieve, args->n_threads);

    /* calculate the primorial */
    mpz_mul(mpz_primorial, sieve->mpz_hash, sieve->mpz_primorial_primes);

    /* run the sieve and chech the candidates */
    sieve_run(sieve, mpz_primorial);
  }

  args->mine = MINING_STOPPED;
  mpz_clear(mpz_primorial);

  free_sieve(sieve);

  return NULL;
}

/**
 * main thread which starts the miner threads
 * and outputs stats
 */
void main_thread(Opts * opts) {
 
  pthread_t *threads = malloc(opts->genproclimit * sizeof(pthread_t));
  MinerArgs *args    = malloc(opts->genproclimit * sizeof(MinerArgs));

  memset(args, 0, opts->genproclimit * sizeof(MinerArgs));

  int i;

  for (i = 0; i < opts->genproclimit; i++) {
    args[i].mine      = MINING_WAIT;
    args[i].opts      = opts;
    args[i].n_threads = opts->genproclimit;
    args[i].id        = i;

    pthread_mutex_init(&args[i].mutex, NULL);
    pthread_create(&threads[i], NULL, primcoin_miner, (void *) &args[i]);
  }

  pthread_t stats;
  pthread_create(&stats, NULL, stats_thread, (void *) args);

  /* connect to pool */
  connect_to(opts);

  /* loop untille programm gets closen */
  while (running) {
    
    switch (recv_work(opts)) {
      
      case WORK_MSG: 
        for (i = 0; i < opts->genproclimit; i++) {
          
          if (args[i].mine == MINING_WAIT)
            args[i].mine = MINING_START;

          pthread_mutex_lock(&args[i].mutex);

          args[i].new_work = 1;
          args[i].sieve.active = 0;

          pthread_mutex_unlock(&args[i].mutex);
        }
        break;

      case SHARE_INFO_MSG:
        /* do nothing special */
        break;

      /* reconnect on failur */
      default:
        if (running)
          connect_to(opts);

    }
  }

  /* shutdown stats thread */
  pthread_join(stats, NULL);

  /* shutdown miner threads */
  for (i = 0; i < opts->genproclimit; i++) {
    args[i].new_work = 0;
    args[i].sieve.active = 0;
  }

  /* fait for threads to finish */
  for (i = 0; i < opts->genproclimit; i++) 
    pthread_join(threads[i], NULL);
  
  free(threads);
  free(args);
}

/**
 * signal handler to exit programm softly
 */
void soft_shutdown(int signum) {
  
  /* not used */
  (void) signum;

  static int shutdown = 0;
  running = 0;

  if (shutdown >= 5) {
    info_msg("\rOK im going to KILL myself!!!\n");
    kill(0, SIGKILL);
  }

  info_msg("\rshuting down...\n");
  shutdown++;
}

/**
 * Start reading here
 */
int main(int argc, char *argv[]) {

   /**
    * indecates that the programm shoud runn
    */
   running = 1;

  /* set signal handler for soft shutdown */
  struct sigaction action;
  memset(&action, 0, sizeof(struct sigaction));

  action.sa_handler = soft_shutdown;
  sigaction(SIGINT,  &action, NULL);
  sigaction(SIGTERM, &action, NULL);

  /* continue mining if terminal lost connection */
  signal(SIGHUP,  SIG_IGN);
  signal(SIGPIPE, SIG_IGN); // send if keepalive probe failed
  signal(SIGQUIT, SIG_IGN);

  /* read the comadline options, and initialize program wide parameters */
  Opts *opts = init_opts(argc, argv);

  /* test */
  printf("Options:\n"
         "  poolfee:            %d\n"
         "  poolip:             %s\n"
         "  poolport:           %d\n"
         "  pooluser:           %s\n"
         "  poolpassword:       %s\n"
         "  genproclimit:       %d\n"
         "  minerid:            %d\n"
         "  n_sieve_extensions: %d\n"
         "  n_sieve_percentage: %d\n"
         "  sievesize:          %d\n"
         "  n_primes_in_hash:   %d\n"
         "  cachebits:          %d\n"
         "  verbose:            %d\n"
         "  stats_interval:     %d\n"
         "  poolshare:          %d\n",
         opts->poolfee,
         opts->poolip,
         opts->poolport,
         opts->pooluser,
         opts->poolpassword,
         opts->genproclimit,
         opts->minerid,
         opts->n_sieve_extensions,
         opts->n_sieve_percentage,
         opts->sievesize,
         opts->n_primes_in_hash,
         opts->cachebits,
         opts->verbose,
         opts->stats_interval,
         opts->poolshare);

  /* start mining */
  main_thread(opts);

  free_opts(opts);
  free(opts);

  return 0;
}
