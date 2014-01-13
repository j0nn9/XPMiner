/**
 * Implementation of an Primecoin (XPM) Miner
 * (main this is weher it begins)
 */
#include <stdio.h>
#include "options.h"

/**
 * Start reading here
 */
int main(int argc, char *argv[]) {

  /* read the comadline options */
  CmdOpts *opts = get_opts(argc, argv);
  

  /* test */
  printf("Options:\n"
         "  poolfee:      %d\n"
         "  poolip:       %s\n"
         "  poolport:     %d\n"
         "  pooluser:     %s\n"
         "  poolpassword: %s\n",
         opts->poolfee,
         opts->poolip,
         opts->poolport,
         opts->pooluser,
         opts->poolpassword);

  return 0;
}
