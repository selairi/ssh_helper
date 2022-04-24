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

#ifndef __THREADSHAREDDATA_H__
#define __THREADSHAREDDATA_H__

#include <pthread.h>
#include <memory>
#include <tuple>
#include <pthread.h>
#include <semaphore.h>
#include "configfileparser.h"
#include "p2pdata.h"

class ThreadSharedData {
  public:
    ThreadSharedData(std::shared_ptr<ConfigItemVector> scripts);
    ~ThreadSharedData();

    std::shared_ptr<ConfigItemVector> getScripts();
    std::string id_session;
    /** Returns seeds for file with md5. if ok == false, no seeds are available. 
     * The file must be send to the first seed. 
     */
    std::tuple<bool /*ok*/, std::shared_ptr<P2PData> > 
      getSeeds(std::string md5);
    
    /** Returns a semaphore for monitor pointer.
     * if semophore is not init, value is taken as init value.
     */
    sem_t *getSemaphore(intptr_t monitor, int value);
  private:
    std::shared_ptr<ConfigItemVector> mScripts; // Array of scripts
    std::map<std::string /*md5*/, std::shared_ptr<P2PData> > p2pSeeds;
    std::map<intptr_t /*monitor*/, sem_t* /*semaphore*/> monitorSemaphores;
    pthread_mutex_t mutex;
};

#endif
