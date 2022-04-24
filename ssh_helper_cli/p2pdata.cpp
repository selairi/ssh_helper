/*
 * (c)GPL3
 *
 * Copyright: 2022 P.L. Lucas <selairi@gmail.com>
 * 
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along with 
 * this program. If not, see <https://www.gnu.org/licenses/>. 
 */

#include "p2pdata.h"
#include "simpleexception.h"

P2PData::P2PData()
{
  if(pthread_mutex_init(&mutex, NULL) != 0) 
    throw(SimpleException("Error: mutex init failed\n"));
  if(sem_init(&semaphore, 0, 0) != 0)
      throw(SimpleException("Error: Semaphore cannot be init."));
}

P2PData::~P2PData()
{
  pthread_mutex_destroy(&mutex);
  sem_destroy(&semaphore);
}


P2PSeed P2PData::getSeed()
{
  sem_wait(&semaphore);
  P2PSeed client = nullptr;
  int rc = pthread_mutex_lock(&mutex);
  if(rc)
    throw(SimpleException("Error: Mutex cannot be locked."));
  if(seeds.size() > 0) {
    client = seeds[0];
    seeds.pop_front();
  }
  rc = pthread_mutex_unlock(&mutex);
  if(rc)
    throw(SimpleException("Error: Mutex cannot be unlocked."));
  return client;
}


void P2PData::addSeed(P2PSeed client)
{
  int rc = pthread_mutex_lock(&mutex);
  if(rc)
    throw(SimpleException("Error: Mutex cannot be locked."));
  seeds.push_back(client);
  rc = pthread_mutex_unlock(&mutex);
  if(rc)
    throw(SimpleException("Error: Mutex cannot be unlocked."));
  sem_post(&semaphore);
}
