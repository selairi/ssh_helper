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

#ifndef __P2PDATA_H__
#define __P2PDATA_H__

#include <pthread.h>
#include <map>
#include <deque>
#include <semaphore.h>
#include <memory>
#include <string>

struct _P2PSeed
{
  std::string user, host, password, path;
};

typedef std::shared_ptr<_P2PSeed> P2PSeed ;


class P2PData
{
  public:
    P2PData();
    ~P2PData();
  
    /** Returns a "user@host" string
     */
    P2PSeed getSeed();
    /** Client must be a "user@host" string
     */
    void addSeed(P2PSeed client);
  private:
    pthread_mutex_t mutex;
    std::deque<P2PSeed> seeds;
    sem_t semaphore;
};

#endif
