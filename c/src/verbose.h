/**
 * heraderfile for threadsave verbose output
 */
#ifndef __VERBOSE_H__
#define __VERBOSE_H__

#include "main.h"

/**
 * print an error message 
 */
void errno_msg(char *msg);

/**
 * printf an formated string 
 */
void info_msg(char *format, ...);

/**
 * generate and print statistics
 */ 
void print_stats(MiningStats stats, Sieve *sieves, uint32_t n_threads);

#endif /* __VERBOSE_H__ */
