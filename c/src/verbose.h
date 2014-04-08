/**
 * heraderfile for threadsave verbose output
 */
#ifndef __VERBOSE_H__
#define __VERBOSE_H__

#include <gmp.h>
#include <inttypes.h>

#include "main.h"

/**
 * print an errno message 
 */
void errno_msg(char *msg);

/**
 * print an error message 
 */
void error_msg(char *format, ...);

/**
 * printf an formated string 
 */
void info_msg(char *format, ...);

/**
 * generate and print statistics
 */ 
void print_stats(MinerArgs *stats, uint32_t n_threads);

/**
 * print miner options if verbose is given
 */
void print_options();

/**
 * prints the license and exits the programm
 */
void print_license();

/**
 * print help message and exit
 */
void print_help();

/**
 * prints an mpz value (debugging)
 */
void print_mpz(const mpz_t mpz);

#endif /* __VERBOSE_H__ */
