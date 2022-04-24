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

#include "threadshareddata.h"
#include "simpleexception.h"


ThreadSharedData::ThreadSharedData(std::shared_ptr<ConfigItemVector> scripts)
{
  mScripts = scripts;
  if(pthread_mutex_init(&mutex, NULL) != 0) 
    throw(SimpleException("Error: mutex init failed\n"));
}


ThreadSharedData::~ThreadSharedData()
{
  pthread_mutex_destroy(&mutex);
  for (const auto& [key, sem] : monitorSemaphores) {
    sem_destroy(sem);
  }
}

std::shared_ptr<ConfigItemVector> ThreadSharedData::getScripts()
{
  return mScripts;
}

std::tuple<bool /*ok*/, std::shared_ptr<P2PData> > ThreadSharedData::getSeeds(std::string md5)
{
  std::shared_ptr<P2PData> seeds;
  bool ok = false;
  pthread_mutex_lock(&mutex);
  if(p2pSeeds.contains(md5)) {
    seeds = p2pSeeds[md5];
    ok = true;
  } else {
    p2pSeeds[md5] = seeds = std::make_shared<P2PData>(); 
  }
  pthread_mutex_unlock(&mutex);
  return std::make_tuple(ok, seeds);
}

sem_t *ThreadSharedData::getSemaphore(intptr_t monitor, int value)
{
  if(monitorSemaphores.contains(monitor))
    return monitorSemaphores[monitor];
  else {
    pthread_mutex_lock(&mutex);
    sem_t *sem = new sem_t;
    if(sem_init(sem, 0, value) != 0) {
      pthread_mutex_unlock(&mutex);
      throw(SimpleException("Error: Semaphore cannot be init."));
    }
    monitorSemaphores[monitor] = sem;
    pthread_mutex_unlock(&mutex);
    return sem;
  }
}

