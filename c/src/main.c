/**
 * Implementation of an Primecoin (XPM) Miner
 * (main this is weher it begins)
 */
#include <stdio.h>
#include <stdlib.h>
#include "options.h"
#include "block.h"

/**
 * Start reading here
 */
int main(int argc, char *argv[]) {

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

  BlockHeader header;
  uint8_t hash[SHA256_DIGEST_LENGTH];
  header_set_null(&header);

  mine_header_hash(&header, opts, hash);

  printf("\nheader_hash: ");
  mpz_out_str(stdout, 10, opts->mpz_hash);
  printf("\n");

  free_opts(opts);
  free(opts);

  return 0;
}
