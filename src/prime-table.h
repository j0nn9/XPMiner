/**
 * Implementation of the default Sieve of Eratosthenes to generat 
 * a prime table including all primes till a given number starting by 2.
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
#ifndef __PRIME_TABLE_H__
#define __PRIME_TABLE_H__

#include <inttypes.h>
#include <gmp.h>

#include "main.h"

/**
 * Primtable with len primes
 */
struct PrimeTable {
  uint32_t *ptr;
  uint32_t len;
};

/**
 * 
 */
PrimeTable *gen_prime_table(uint32_t sieve_size);

/**
 * creat an so called primorial 
 * (a composite number out of a given range of primes)
 */
void primorial(PrimeTable *primes, 
               mpz_t mpz_primorial, 
               uint32_t start, 
               uint32_t end);

/**
 * creat an so called primorial 
 * (a composite number out of a given range of primes)
 */
void int_primorial(PrimeTable *primes, uint32_t *primorial, uint32_t end);

#endif /* __PRIME_TABLE_H__ */
