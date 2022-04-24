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

#ifndef __CLIENTTHREAD_H__
#define __CLIENTTHREAD_H__

#include "threadshareddata.h"
#include "sshptr.h"
#include <pthread.h>
#include <iostream>

class ClientThread {
  public:
    ClientThread(std::shared_ptr<ThreadSharedData> threadSharedData, std::string host, int port, std::string user, std::string password, std::ostream *log_output);
    ~ClientThread();

    bool connect();
    
    void run(std::shared_ptr<ConfigItemVector> scripts = nullptr);
    void clean_temp();

    pthread_t *getThread();
    
    static void *start(void *data);
  private:
    std::shared_ptr<ThreadSharedData> mThreadSharedData;
    std::string host, user, password;
    int port;
    std::shared_ptr<SshPtr> ssh;
    bool is_connected;
    pthread_t *thread;
    pthread_mutex_t *mutex;
    std::ostream *log_output;

    void save_log(std::shared_ptr<ConfigItemMap> map, std::string log, const int &rc);
};

#endif
