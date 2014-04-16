/**
 * Implementation of the getwork protocol for pool mining.
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
#ifndef __NET_H__
#define __NET_H__

#include "main.h"

/**
 * the Message types we can receive from the server
 */
#define WORK_MSG 0
#define SHARE_INFO_MSG 1


/**
 * (re)connect to the given (pool)ip and (pool)port
 */
void connect_to_pool();

/**
 * receive work from server
 * returns
 *  the message type or
 *  -1 on failure 
 */
int recv_work(MinerArgs *args);

/**
 * sends a valid share (block header) to the server
 */
void submit_share(BlockHeader *share, 
                  char type, 
                  uint32_t difficulty);

#endif /* __NET_H__ */
