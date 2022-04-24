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

#include "manager.h"
#include "simpleexception.h"
#include <sstream>
#include <fstream>
#include <stdlib.h>
#include <time.h>

Manager::Manager(std::shared_ptr<ConfigItemVector> scripts_and_host, std::string password, bool no_multi, std::filesystem::path &log_path)
{
  this->mScripts_and_host = scripts_and_host;
  this->password = password;
  this->no_multi = no_multi;
  this->log_path = log_path;
}


void Manager::checkKeys()
{
  // Check public and private keys
  char *home = getenv("HOME");
  if(home == nullptr) {
    throw(SimpleException("$HOME environment variable is not defined."));
  }

  std::filesystem::path path(home);
  path /= ".ssh";
  path /= "id_rsa";
  if(std::filesystem::exists(path)) {
    std::filesystem::path path2(home);
    path2 /= ".ssh";
    path2 /= "id_rsa.pub";
    if(std::filesystem::exists(path2)) 
      return;
  }

  // No public keys. Generate new ones.
  system("printf \"\n\n\n\" | ssh-keygen -f ~/.ssh/id_rsa");
}


void Manager::makeIdSession()
{
  std::string id;
  time_t now = time(NULL);
  struct tm t, *p;
  p = localtime_r(&now, &t);
  if(p != NULL) {
    std::ifstream hostname_file;
    hostname_file.open("/etc/hostname");
    if(!hostname_file.is_open()) {
      throw("Error: /etc/hostname cannot be opened.");
    }
    std::string hostname;
    getline(hostname_file, hostname);
    hostname_file.close();
    char *local_user = getenv("USER");
    if(local_user == NULL)
      throw("Error: USER environmet variable cannot be read.");
    srandom(now);
    id += std::to_string(t.tm_year + 1901) + "-" 
      + (t.tm_mon  < 9  ? "0" : "") + std::to_string(t.tm_mon + 1) + "-" 
      + (t.tm_mday < 10 ? "0" : "") + std::to_string(t.tm_mday) + "-" 
      + (t.tm_hour < 10 ? "0" : "") + std::to_string(t.tm_hour) + ":" 
      + (t.tm_min  < 10 ? "0" : "") + std::to_string(t.tm_min) + ":" 
      + (t.tm_sec  < 10 ? "0" : "") + std::to_string(t.tm_sec) 
      + "·" + std::to_string(random()) 
      + "·" + std::string(local_user)
      + "@" + hostname;
  } else {
    throw("Error: Date and time cannot be got.");
  }
  mThreadSharedData->id_session = id;
  std::cout << "\033[1mSession id: \033[0m\033[4m" << id << "\033[0m"<< std::endl;
}


void Manager::run()
{
  std::shared_ptr<ConfigItemVector> scripts, hosts;
  for(auto item : mScripts_and_host->getValue()) {
    std::string tag;
    std::shared_ptr<ConfigItem> value;
    std::tie(tag, value) = item;
    if(tag == "scripts") {
      if(value->getType() == ConfigItemType::VECTOR)
        scripts = std::static_pointer_cast<ConfigItemVector>(value);
      else
        throw(SimpleException("Error: \"scripts\" must be a vector (scripts +)."));
    } else if(tag == "hosts") {
      if(value->getType() == ConfigItemType::VECTOR)
        hosts = std::static_pointer_cast<ConfigItemVector>(value);
      else
        throw(SimpleException("Error: \"hosts\" must be a vector (hosts +)."));
    } else {
      throw(SimpleException("Error: Tag " + tag + " doesn't be at root."));
    }
  }

  mThreadSharedData = std::make_shared<ThreadSharedData>(scripts);
  makeIdSession();

  // Launch clients
  for(std::tuple<std::string, std::shared_ptr<ConfigItem> > item : hosts->getValue()) {
    std::string tag;
    std::shared_ptr<ConfigItem> value;
    std::tie(tag, value) = item;
    if(tag == "host" && value->getType() == ConfigItemType::MAP) {
      std::shared_ptr<ConfigItemMap> map_ptr = std::static_pointer_cast<ConfigItemMap>(value);
      std::map<std::string, std::shared_ptr<ConfigItem> > map = map_ptr->getValue();
      std::string host, user, password;
      int port = 22;
      if(map.contains("host")) {
        std::shared_ptr<ConfigItem> ptr = map["host"];
        if(ptr->getType() == ConfigItemType::STRING) {
          std::shared_ptr<ConfigItemString> str = static_pointer_cast<ConfigItemString>(ptr);
          host = str->getValue();
        }
      }
      if(map.contains("user")) {
        std::shared_ptr<ConfigItem> ptr = map["user"];
        if(ptr->getType() == ConfigItemType::STRING) {
          std::shared_ptr<ConfigItemString> str = static_pointer_cast<ConfigItemString>(ptr);
          user = str->getValue();
        }
      }
      if(map.contains("password")) {
        std::shared_ptr<ConfigItem> ptr = map["password"];
        if(ptr->getType() == ConfigItemType::STRING) {
          std::shared_ptr<ConfigItemString> str = static_pointer_cast<ConfigItemString>(ptr);
          password = str->getValue();
        }
      }
      if(map.contains("port")) {
        std::shared_ptr<ConfigItem> ptr = map["port"];
        if(ptr->getType() == ConfigItemType::STRING) {
          std::shared_ptr<ConfigItemString> str = static_pointer_cast<ConfigItemString>(ptr);
          std::stringstream buf(str->getValue());
          buf >> port;
        }
      }

      if(host.empty()) {
        throw(SimpleException("\"host\" tag is empty."));
      }

      if(user.empty()) {
        throw(SimpleException("\"user\" tag is empty."));
      } 

      if(password.empty()) {
        password = this->password;
      }

      // Open file log: user@host.txt
      std::filesystem::path path(log_path);
      std::filesystem::path log_file(user + "@" + host + ".txt");
      path /= log_file;
      std::ofstream *log_stream = new std::ofstream;
      log_stream->open(path);
      if(!log_stream->is_open())
        throw(SimpleException(std::string("Log file ") + path.c_str() + std::string(" cannot be opened.")));
      ClientThread *client_ptr = new ClientThread(mThreadSharedData, host, port, user, password, log_stream);
      std::shared_ptr<ClientThread> client(client_ptr);

      clients.push_back(client);      

      pthread_create(client->getThread(), NULL, &ClientThread::start, (void*)client_ptr);
      if(no_multi)
        pthread_join(*client->getThread(), NULL);
    }
  }

  if(!no_multi) {
    for(std::shared_ptr<ClientThread> client : clients) {
      pthread_join(*client->getThread(), NULL);
    }
  }

  std::cout << "Cleaning temp folder..." << std::endl;
  for(std::shared_ptr<ClientThread> client : clients) {
    client->clean_temp();
  }
  std::cout << "done." << std::endl;

}
