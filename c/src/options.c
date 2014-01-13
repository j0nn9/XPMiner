/**
 * Code for parsing the comand line options
 */
#include <stdlib.h>
#include <getopt.h>

#include "options.h"

/**
 * macros to access the readed opts
 */
#define POOLFEE       1
#define POOLIP        2
#define POOLPORT      3
#define POOLUSER      4
#define POOLPASSWORD  5

/**
 * the avilabe comand line options
 */
static struct option long_options[] = {
  { "poolfee",      required_argument, 0, POOLFEE      },
  { "poolip",       required_argument, 0, POOLIP       },
  { "poolport",     required_argument, 0, POOLPORT     },
  { "pooluser",     required_argument, 0, POOLUSER     },
  { "poolpassword", required_argument, 0, POOLPASSWORD },
  { 0,              0,                 0, 0            }
};


/**
 * read the comand line options into a CmdOpts struct
 */
CmdOpts *get_opts(int argc, char *argv[]) {

  CmdOpts *cmd_opts = malloc(sizeof(CmdOpts));
 
  int opt       = 0;
  int opt_index = 0;
 
  /* loop */
  for (;;) {
   
    opt = getopt_long_only(argc, argv, "", long_options, &opt_index);
    
    /* detect end of options */
    if (opt == -1)
      break;
    
    /* parse the option */
    switch (opt) {
    
      case POOLFEE:
        cmd_opts->poolfee = atoi(optarg);
        break;
    
      case POOLIP:
        cmd_opts->poolip = optarg;
        break;
    
      case POOLPORT:
        cmd_opts->poolport = atoi(optarg);
        break;
    
      case POOLUSER:
        cmd_opts->pooluser = optarg;
        break;
    
      case POOLPASSWORD:
        cmd_opts->poolpassword = optarg;
        break;
    }
  }

  return cmd_opts;
}
