/**
 * some utils
 */
#ifndef __UTILS_H__
#define __UTILS_H__

/**
 * exit on error
 */
#define perror(msg)         \
do {                        \
  perror(msg);              \
  exit(EXIT_FAILURE);       \
} while (0)

#endif /* __UTILS_H__ */
