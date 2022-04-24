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

#include "clientthread.h"
#include "simpleexception.h"
#include <fstream>
#include <filesystem>
#include <time.h>
#include <regex>


const std::string ASKPASS_PY = R"(#!/usr/bin/env python3

import os
import sys

_b = sys.version_info[0] < 3 and (lambda x:x) or (lambda x:x.encode('utf-8'))

def ssh_exec_pass(password, args, capture_output=False):
    '''
        Wrapper around openssh that allows you to send a password to
        ssh/sftp/scp et al similar to sshpass.

        Not super robust, but works well enough for most purposes. Typical
        usage might be::

            ssh_exec_pass('p@ssw0rd', ['ssh', 'root@1.2.3.4', 'echo hi!'])

        :param args: A list of args. arg[0] must be the command to run.
        :param capture_output: If True, suppresses output to stdout and stores
                               it in a buffer that is returned
        :returns: (retval, output)

        *nix only, tested on linux and OSX. Python 2.7 and 3.3+ compatible.
    '''

    import pty, select

    # create pipe for stdout
    stdout_fd, w1_fd = os.pipe()
    stderr_fd, w2_fd = os.pipe()

    pid, pty_fd = pty.fork()
    if not pid:
        # in child
        os.close(stdout_fd)
        os.close(stderr_fd)
        os.dup2(w1_fd, 1)    # replace stdout on child
        os.dup2(w2_fd, 2)    # replace stderr on child
        os.close(w1_fd)
        os.close(w2_fd)

        os.execvp(args[0], args)

    os.close(w1_fd)
    os.close(w2_fd)

    output = bytearray()
    rd_fds = [stdout_fd, stderr_fd, pty_fd]

    def _read(fd):
        if fd not in rd_ready:
            return
        try:
            data = os.read(fd, 1024)
        except (OSError, IOError):
            data = None
        if not data:
            rd_fds.remove(fd) # EOF

        return data

    # Read data, etc
    try:
        while rd_fds:

            rd_ready, _, _ = select.select(rd_fds, [], [], 0.04)

            if rd_ready:

                # Deal with prompts from pty
                data = _read(pty_fd)
                if data is not None:
                    if b'assword:' in data:
                        os.write(pty_fd, _b(password + '\n'))
                    elif b're you sure you want to continue connecting' in data:
                        os.write(pty_fd, b'yes\n')

                # Deal with stdout
                data = _read(stdout_fd)
                if data is not None:
                    if capture_output:
                        output.extend(data)
                    else:
                        sys.stdout.write(data.decode('utf-8', 'ignore'))

                data = _read(stderr_fd)
                if data is not None:
                    sys.stderr.write(data.decode('utf-8', 'ignore'))
    finally:
        os.close(pty_fd)

    pid, retval = os.waitpid(pid, 0)
    return retval, output

if __name__ == '__main__':
    password = input('')
    retval, _ = ssh_exec_pass(password.strip(), sys.argv[1:], False)
    exit(retval)
)";


ClientThread::ClientThread(std::shared_ptr<ThreadSharedData> threadSharedData, std::string host, int port, std::string user, std::string password, std::ostream *log_output)
{
  mThreadSharedData = threadSharedData;
  this->host = host;
  this->user = user;
  this->password = password;
  this->port = port;
  this->log_output = log_output;
  is_connected = false;
  thread = new pthread_t;
  mutex = new pthread_mutex_t;

  if(pthread_mutex_init(mutex, NULL) != 0) {
    throw(SimpleException("Error: mutex init failed\n"));
  }

}


ClientThread::~ClientThread()
{
  delete thread;
  pthread_mutex_destroy(mutex);
  delete log_output;
}


pthread_t *ClientThread::getThread()
{
  return thread;
}

void *ClientThread::start(void *data)
{
  try {
    ClientThread *client = (ClientThread *)data;
    client->run();
  } catch(SimpleException &error) {
    std::cerr << error.what() << std::endl;
  }
  return nullptr;
}


bool ClientThread::connect()
{
  if(! is_connected) {
    ssh = std::make_shared<SshPtr>(host, port);
    is_connected = ssh->connect(user, password);
  }
  return is_connected;
}

void ClientThread::save_log(std::shared_ptr<ConfigItemMap> map, std::string log, const int &rc)
{
  if(map->getValue().contains("name")) {
    std::shared_ptr<ConfigItem> name_ptr = map->getValue()["name"];
    if(name_ptr->getType() == ConfigItemType::STRING) {
      std::shared_ptr<ConfigItemString> name = static_pointer_cast<ConfigItemString>(name_ptr);
      if(log.empty()) {
        if(rc == 0)
          *log_output << name->getValue() << ": OK" << std::endl;
        else
          *log_output << name->getValue() << ": FAIL" << std::endl;
      } else {
        if(!log.ends_with("\n"))
          log += "\n";
        *log_output << name->getValue() << ": " << log; 
      }
    }
  } else {
    if(log.empty()) {
      if(rc == 0)
        *log_output << ": OK" << std::endl;
      else
        *log_output << ": FAIL" << std::endl;
    } else {
      if(!log.ends_with("\n"))
        log += "\n";
      *log_output << ": " << log; 
    }            
  }
}

void ClientThread::run(std::shared_ptr<ConfigItemVector> scripts)
{
  std::string log;
  std::cout << "Client " << host << std::endl; 
  std::string shared_folder = "/home/" + user + "/.local/share/ssh_helper_temp/" + mThreadSharedData->id_session;
  
  if(connect()) {
    if(scripts == nullptr) {
      // Set shared folder
      {
        int rc;
        std::string log;
        std::tie(rc, log) = ssh->exec("mkdir -p " + shared_folder); 
        std::tie(rc, log) = ssh->exec("chmod 700 " + shared_folder);
      }

      // Send manager public keys
      {
        int rc;
        std::string log;
        char *local_user = getenv("USER");
        if(local_user == NULL)
          throw("Error: USER environmet variable cannot be read.");
        std::string public_key_file("/home/" + std::string(local_user) + "/.ssh/id_rsa.pub");
        std::string public_key;
        std::ifstream public_key_stream;
        public_key_stream.open(public_key_file);
        if(!public_key_stream.is_open())
          throw("Error: " + public_key_file + " cannot be opened.");
        getline(public_key_stream, public_key);
        public_key_stream.close();

        std::tie(rc, log) = ssh->exec("mkdir -p ~/.ssh"); 
        std::tie(rc, log) = ssh->exec("chmod 700 ~/.ssh");

        // Public key is added to authorized_keys
        // Check if key is not in authorized_keys:
        std::tie(rc, log) = ssh->exec("grep -e '" + public_key + "' ~/.ssh/authorized_keys");
        if(rc != 0) {
          // Add key to authorized_keys
          std::tie(rc, log) = ssh->exec("echo '' >> ~/.ssh/authorized_keys"); // Add a \n to authorized_keys
          std::tie(rc, log) = ssh->exec("echo '" + public_key + "' >> ~/.ssh/authorized_keys");
          if(rc != 0)
            throw("Error: public keys cannot be added to " + user + "@" + host);
        }
      }
    }
    // Run scripts
    if(scripts == nullptr)
      scripts = mThreadSharedData->getScripts();
    for(std::tuple<std::string, std::shared_ptr<ConfigItem> > script : scripts->getValue()) {
      std::string tag;
      std::shared_ptr<ConfigItem> value;
      std::tie(tag, value) = script;
      if(tag == "script" && value->getType() == ConfigItemType::MAP) {
        std::shared_ptr<ConfigItemMap> map = static_pointer_cast<ConfigItemMap>(value);
        if(map->getValue().contains("command")) {
          std::shared_ptr<ConfigItem> command_ptr = map->getValue()["command"];
          if(command_ptr->getType() == ConfigItemType::STRING) {
            // Exec command
            std::shared_ptr<ConfigItemString> command = static_pointer_cast<ConfigItemString>(command_ptr);
            int rc;
            log.clear();
            std::string sudo = ConfigFileParser::getMapValue(map, "sudo");
            if(!sudo.empty()) {
              std::cout << "sudo " << command->getValue() << std::endl;
              std::string sudo_script_path =shared_folder + "/sudo_script.sh"; 
              ssh->ssh_write_to_file(command->getValue(), sudo_script_path);
              std::tie(rc, log) = ssh->exec_sudo("bash '" + sudo_script_path + "'");
            } else {
              std::cout << "$ " << command->getValue() << std::endl;
              std::tie(rc, log) = ssh->exec(command->getValue());
            }
            // Save log
            save_log(map, log, rc);
          }
        }
      } else if(tag == "upload" && value->getType() == ConfigItemType::MAP) {
        std::cout << user << "@" << host << " scp " << std::endl;
        std::shared_ptr<ConfigItemMap> map = ConfigFileParser::getMap(value);
        std::string orig = strip(ConfigFileParser::getMapValue(map, "orig"));
        if(orig.empty())
          throw(SimpleException("SCP Error: orig tag is missing."));
        std::string dest = strip(ConfigFileParser::getMapValue(map, "dest"));
        if(dest.empty())
          throw(SimpleException("SCP Error: dest tag is missing."));
        std::string final_user(user); 
        final_user = strip(ConfigFileParser::getMapValue(map, "user"));
        if(final_user.empty())
          final_user = user;
        std::string md5 = strip(ConfigFileParser::getMapValue(map, "md5"));
        if(md5.empty())
          throw(SimpleException("SCP Error: md5 tag is missing."));

        if(dest.starts_with("~"))
          dest = std::regex_replace(dest, std::regex("^~"), "/home/" + final_user);

        std::filesystem::path orig_path(orig);
        std::filesystem::path orig_filename = orig_path.filename();
        std::string dest_path = dest + "/" + orig_filename.string();

        std::cout << user << "@" << host << " scp file " << dest_path  << std::endl;

        int rc;
        std::string log;
        std::shared_ptr<char*> output;
        bool file_on_remote_host = false;
        if(final_user == user)
          std::tie(rc, output, log) = ssh->exec_get_output("md5sum -b '" + dest_path + "' | awk '{print $1}'");
        else
          std::tie(rc, output, log) = ssh->exec_sudo_get_output("md5sum -b '" + dest_path + "' | awk '{print $1}'");
        if(rc != -1) { // If rc == -1, user is not a sudoer. md5sum command fails.

          if(rc == 0) {
            // The file "dest" exists on host.
            std::string md5_host(*output);
            if(md5 == strip(md5_host)) {
              // The file is already on remote host, do nothing
              std::cout << user << "@" << host << " file " + dest_path + " is on remote host." << std::endl;
              save_log(map, log, rc);
              file_on_remote_host = true;
            }
          }
          if(! file_on_remote_host) {
            std::cout << user << "@" << host << " file " + dest_path + " is not on remote host." << std::endl;
            std::shared_ptr<P2PData> seeds;
            bool ok;
            std::tie(ok, seeds)  = mThreadSharedData->getSeeds(md5);
            if(! ok) {
              // The file must be uploaded
              std::cout << user << "@" << host << " no seeds for file " + dest_path << std::endl;
              std::tie(rc, log) = ssh->exec("mkdir -p " + shared_folder + "\"/" + dest + "\"");
              if(rc != 0) {
                std::cout << user << "@" << host << " folder " << shared_folder + "/" + dest << " cannot be maked." << std::endl;
                log = "mkdir of shared folder failed.";
                save_log(map, log, rc);
              }
              std::cout << user << "@" << host << " uploading file " << orig << " to " << shared_folder + "/" + dest_path << std::endl;
              try {
                ssh->scp_write(orig, shared_folder + "/" + dest_path);
                // File uploaded to shared folder. Add as seed
                P2PSeed seed = std::make_shared<_P2PSeed>();
                seed->user = user;
                seed->host = host;
                seed->password = password;
                seed->path = shared_folder + "/" + dest_path;
                seeds->addSeed(seed);
                // Copy file to destination
                if(final_user == user) {
                  std::tie(rc, log) = ssh->exec("mkdir -p \"/" + dest + "\"");
                  std::tie(rc, log) = ssh->exec("cp "+ shared_folder + "/\"" + dest_path + "\" '"+ dest + "'");
                  std::tie(rc, log) = ssh->exec("chmod 600 '" + dest_path + "'");
                } else {
                  std::tie(rc, log) = ssh->exec_sudo(" -u " + final_user + "mkdir -p \"/" + dest + "\"");
                  std::tie(rc, log) = ssh->exec_sudo("cp '"+ shared_folder + "/" + dest_path + "' '"+ dest + "'");
                  std::tie(rc, log) = ssh->exec_sudo("chmod 600 '" + dest_path + "'");
                  std::tie(rc, log) = ssh->exec_sudo("chown " + final_user + " '" + dest_path + "'");
                }
                save_log(map, log, rc);
              } catch (SshException &error) {
                log = error.what();
                save_log(map, log, 1);
              }
            } else {
              std::cout << user << "@" << host << " waiting for seeds for file " + dest_path << std::endl;
              // Wait to available seed
              P2PSeed seed = seeds->getSeed();
              if(seed == nullptr) {
                std::cerr << "seed == null. No seeds available." << std::endl;
                exit(1);
              }
              // mkdir shared folder
              std::tie(rc, log) = ssh->exec("mkdir -p \"/" + shared_folder + "/" + dest + "\"");
              // Seed available. Copy from file from seed.
              // Upload askpass.py to run scp
              std::tie(rc, log) = ssh->exec("head -c " + std::to_string(ASKPASS_PY.size()) + " - > " + shared_folder + "/askpass.py", ASKPASS_PY);
              std::string seed_uri = seed->user + "@" + seed->host + ":" + seed->path;
              std::string command = "python3 " + shared_folder + "/askpass.py"+ " scp '" + seed_uri + "' '" + shared_folder + "/" + dest_path + "'";
              std::tie(rc, log) = ssh->exec(command, seed->password + "\n");
              if(rc == 0) {
                // File uploaded. Add seeds
                P2PSeed nseed = std::make_shared<_P2PSeed>();
                nseed->user = user;
                nseed->host = host;
                nseed->password = password;
                nseed->path = shared_folder + "/" + dest_path;
                seeds->addSeed(nseed);
                seeds->addSeed(seed);
                // Copy file to destination
                if(final_user == user) {
                  std::tie(rc, log) = ssh->exec("mkdir -p \"/" + dest + "\"");
                  std::tie(rc, log) = ssh->exec("cp "+ shared_folder + "/\"" + dest_path + "\" '"+ dest + "'");
                  std::tie(rc, log) = ssh->exec("chmod 600 '" + dest_path + "'");
                } else {
                  std::tie(rc, log) = ssh->exec_sudo(" -u " + final_user + "mkdir -p \"/" + dest + "\"");
                  std::tie(rc, log) = ssh->exec_sudo("cp '"+ shared_folder + "/" + dest_path + "' '"+ dest + "'");
                  std::tie(rc, log) = ssh->exec_sudo("chmod 600 '" + dest_path + "'");
                  std::tie(rc, log) = ssh->exec_sudo("chown " + final_user + " '" + dest_path + "'");
                }
              } else {
                seeds->addSeed(seed); 
              }
              if(final_user == user)
                std::tie(rc, output, log) = ssh->exec_get_output("md5sum -b '" + dest_path + "' | awk '{print $1}'");
              else
                std::tie(rc, output, log) = ssh->exec_sudo_get_output("md5sum -b '" + dest_path + "' | awk '{print $1}'");
              std::string md5_host(*output);
              if(md5 == strip(md5_host))
                save_log(map, log, 0);
              else
                save_log(map, std::string("Error."), 1);
            }
          }
        } else { // User is not a sudoer
          save_log(map, log, rc);
        }
      } else  if(tag == "download" && value->getType() == ConfigItemType::MAP) {
        std::shared_ptr<ConfigItemMap> map = ConfigFileParser::getMap(value);
        std::string orig = strip(ConfigFileParser::getMapValue(map, "orig"));
        if(orig.empty())
          throw(SimpleException("Download Error: orig tag is missing."));
        std::string dest = strip(ConfigFileParser::getMapValue(map, "dest"));
        if(dest.empty())
          throw(SimpleException("Download Error: dest tag is missing."));
        
        if(dest.starts_with("~"))
          dest = std::regex_replace(dest, std::regex("^~"), std::string("/home/") + getenv("USER"));
        if(orig.starts_with("~"))
          orig = std::regex_replace(orig, std::regex("^~"), std::string("/home/") + user);

        bool download_ok = false;
        int rc;
        std::string log;
        std::string command = "mkdir -p '" + dest + "'";
        rc = system(command.c_str());
        if(rc == 0) {
          std::tie(rc, log) = ssh->exec("mkdir -p '" + shared_folder + "/" + orig + "'");
          if(rc == 0) {
            std::tie(rc, log) = ssh->exec_sudo("cp -Rf '" + orig + "' '" + shared_folder + "/" + orig + "'");
            if(rc == 0) {
              std::tie(rc, log) = ssh->exec_sudo("chown -R " + user + " '" + shared_folder + "/" + orig + "'");
              if(rc == 0) {
                std::string orig_path = user + "@" + host + ":" + shared_folder + "/" + orig;
                std::string dest_path = dest + "/" + user + "@" + host;
                std::string command = "scp -r '" + orig_path + "' '" + dest_path + "'";
                std::cout << command << std::endl;
                rc = system(command.c_str());
                download_ok = rc == 0;
              }
            }
            std::tie(rc, log) = ssh->exec("rm -Rf '" + shared_folder + "/" + orig + "'");
          }
        }
        if(download_ok) {
          rc = 0;
        } else {
          rc = 1;
          log = "Download failed";
        }
        save_log(map, log, rc);
      } else if(tag == "monitor" && value->getType() == ConfigItemType::MAP) {
        std::shared_ptr<ConfigItemMap> map = ConfigFileParser::getMap(value);
        std::shared_ptr<ConfigItemVector> scripts_ptr = nullptr, scripts_lock_ptr;
        std::string threads = strip(ConfigFileParser::getMapValue(map, "threads"));
        if(threads.empty())
          throw(SimpleException("Monitor Error: threads tag is missing."));
        int nThreads = 1;
        try {
          std::stringstream buff(threads);
          buff >> nThreads;
        } catch (...) {
          throw(SimpleException("Monitor Error: threads tag must be a number."));
        }
        if(map->getValue().contains("scripts_lock")) {
          std::shared_ptr<ConfigItem> ptr = map->getValue()["scripts_lock"];
          if(ptr->getType() != ConfigItemType::VECTOR) {
            throw(SimpleException("Monitor Error: scripts_lock tag must be a vector type \"scripts_lock +\"."));
          } else {
            scripts_lock_ptr = std::static_pointer_cast<ConfigItemVector>(ptr);
          }
        } else {
          throw(SimpleException("Monitor Error: scripts_lock tag is missing."));
        }
        if(map->getValue().contains("scripts")) {
          std::shared_ptr<ConfigItem> ptr = map->getValue()["scripts"];
          if(ptr->getType() != ConfigItemType::VECTOR) {
            throw(SimpleException("Monitor Error: scripts tag must be a vector type \"scripts +\"."));
          } else {
            scripts_ptr = std::static_pointer_cast<ConfigItemVector>(ptr);
          }
        }

        sem_t *sem = mThreadSharedData->getSemaphore(reinterpret_cast<intptr_t>(map.get()), nThreads);
        if(sem_trywait(sem) == 0) {
          // The thread is the monitor.
          // Run lock scripts
          try {
          run(scripts_lock_ptr);
          } catch(SimpleException &error) {
            sem_post(sem);
            throw(error);
          }
          sem_post(sem);
          // Run no lock scripts
          if(scripts_ptr != nullptr)
            run(scripts_ptr);
        } else {
          // This thread cannot enter in the monitor.
          // Run no lock scripts
          if(scripts_ptr != nullptr)
            run(scripts_ptr);
          if(sem_wait(sem) == 0) {
            // Run lock scripts
            try {
              run(scripts_lock_ptr);
            } catch(SimpleException &error) {
              sem_post(sem);
              throw(error);
            }
            sem_post(sem);
          } else {
            throw(SimpleException("Monitor Error: Semaphore is in wrong state."));
          }
        }

      }
    }
  }
}


void ClientThread::clean_temp()
{
  if(is_connected) {
    std::string shared_folder = "~/.local/share/ssh_helper_temp/" + mThreadSharedData->id_session;
    int rc;
    std::string log;
    std::shared_ptr<char*> output;
    std::tie(rc, log) = ssh->exec("rm -Rf " + shared_folder); 
    // Clean old temp folders
    std::tie(rc, output, log) = ssh->exec_get_output("ls ~/.local/share/ssh_helper_temp");
    std::stringstream buffer(*output);
    std::string path;
    time_t now = time(NULL);
    while(std::getline(buffer, path)) {
      struct tm t;
      localtime_r(&now, &t);
      char *ok = strptime(path.substr(0, 10).c_str(), "%Y-%m-%d", &t);
      if(ok != NULL) {
        time_t tp = mktime(&t);
        double diff = difftime(now, tp);
        std::cout << path << " " << path.substr(0, 10).c_str() << " tp=" << tp << " now=" << now << " diff=" << diff << " > " << 3600*24*5 << std::endl;
        if(diff > 3600 * 24 * 5) {
          // 5 days old folders will be erased
          std::tie(rc, output, log) = ssh->exec_get_output("rm -Rf ~/.local/share/ssh_helper_temp/" + path);
        }
      }
    }
  }
}

