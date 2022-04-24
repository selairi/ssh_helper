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

#ifndef __MANAGER_H__
#define __MANAGER_H__

#include "configfileparser.h"
#include "threadshareddata.h"
#include "clientthread.h"
#include <filesystem>

class Manager
{
  public:
    Manager(std::shared_ptr<ConfigItemVector> scripts_and_host, std::string password, bool no_multi, std::filesystem::path &log_path);

    void run();
    /** Checks ssh public and private keys located at "~/.ssh/id_rsa"
     */
    void checkKeys();
    /** Build ID session and saves in ThreadSharedData.
     */
    void makeIdSession();
  private:
    std::shared_ptr<ConfigItemVector> mScripts_and_host;
    std::string password;
    bool no_multi;
    std::shared_ptr<ThreadSharedData> mThreadSharedData;
    std::vector<std::shared_ptr<ClientThread> > clients;
    std::filesystem::path log_path;
};

#endif
