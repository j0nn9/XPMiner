/**
 * Implementation of an Primecoin (XPM) Miner
 * (main this is weher it begins)
 */
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#define __USE_GNU
#include <pthread.h>
#undef __USE_GNU

#include "main.h"

/**
 * determinates wether the miner should shutdown
 */
static char running = 1;

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
  while (*args->mine == 0)
    pthread_yield();

  /* start mining */
  while (running) {

    /* reset nonce if new work arrieved */
    if (args->new_work) {

      pthread_mutex_lock(&args->mutex); 

      args->new_work = 0;

      /* reinit sieve */
      reinit_sieve(sieve, opts);

      sieve->header.nonce = args->id;

      pthread_mutex_unlock(&args->mutex);
    } else {

      /* reinit sieve */
      reinit_sieve(sieve, opts);
    }

    /* generate a hash divisible by the hash primorial */
    mine_header_hash(sieve, args->n_threads);

    /* generate the primorial */
    mpz_set(mpz_primorial, sieve->mpz_hash);
    primorial(sieve->primes, 
              mpz_primorial, 
              opts->n_primes_in_hash + 1, 
              opts->n_primes_in_primorial);

    /* run the sieve and chech the candidates */
    sieve_run(sieve, mpz_primorial);
  }

  mpz_clear(mpz_primorial);

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

  /* indecates that the miner can start */
  char mine = 0;

  for (i = 0; i < opts->genproclimit; i++) {
    args[i].mine = &mine;
    pthread_mutex_init(&args[i].mutex, NULL);
    pthread_create(&threads[i], NULL, primcoin_miner, (void *) &args[i]);
  }

  /* connect to pool */
  connect_to(opts);

  /* loop untille programm gets closen */
  while (running) {
    
    switch (recv_work(opts)) {
      
      case WORK_MSG: 
        mine = 1;

        for (i = 0; i < opts->genproclimit; i++) {
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
        connect_to(opts);

    }
  }

  /* shutdown threads */
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
  (void) signum;
  running = 0;

  info_msg("shuting down...\n");
}

/**
 * Start reading here
 */
int main(int argc, char *argv[]) {

  /* set signal handler for soft shutdown */
  struct sigaction action;
  memset(&action, 0, sizeof(struct sigaction));

  action.sa_handler = soft_shutdown;
  sigaction(SIGINT,  &action, NULL);
  sigaction(SIGTERM, &action, NULL);

  /* continue mining if terminal lost connection */
  signal(SIGHUP,  SIG_IGN);
  signal(SIGPIPE, SIG_IGN);
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
         "  n_primes_in_hash:   %d\n",
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
         opts->n_primes_in_hash);

  free_opts(opts);
  free(opts);

  return 0;
}
