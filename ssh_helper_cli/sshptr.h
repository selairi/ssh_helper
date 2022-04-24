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

#ifndef __SSHCPP_H__
#define __SSHCPP_H__
#include <libssh/libssh.h>
#include <stdlib.h>
#include <memory>
#include <cstdio>
#include <string>
#include <tuple>
#include <exception>

class SshException;

/** Simple wrap for ssh_session C struct. 
 *
 *  std::string host("localhost");
 *  std::shared_ptr<SshPtr> ssh = std::make_shared<SshPtr>(host, 22);
 *  bool ok = ssh->connect(std::string("user"), std::string("password"));
 *  if(ok) {
 *    try {
 *      int status;
 *      status = ssh->exec_sudo(std::string("ls -l /hshkdfjks"));
 *      std::shared_ptr<char *> output;
 *      std::tie(status, output) = ssh->exec_get_output(std::string("ls -l"));
 *      printf("\n>>>>%s<<<\n", *output);
 *      status = ssh->exec("ls /sfgsdfsfds"); 
 *      printf("status %d\n", status);
 *      std::tie(status, output) = ssh->exec_sudo_get_output(std::string("ls -l"));
 *      printf("\n>>>>%s<<<\n", *output);
 *      ssh->scp_write(std::string("/home/lucas/sdcard/prog/libssh/pruebas/Makefile"), std::string("/home/lucas/Descargas/Makefile.txt"));
 *    } catch(SshException &err) {
 *      fprintf(stderr, "Error: %s\n", err.what());
 *    }
 *    
 *  }
 *
 */
class SshPtr {
  public:
    SshPtr(std::string host, int port);
    ~SshPtr();

    inline ssh_session get() {return session;}
    [[nodiscard]] bool connect(std::string user, std::string password);
    /** Run remote command.
     * @return status get status of output command and log info.*/
    [[nodiscard]] std::tuple<int /*status*/, std::string /*log*/> 
      exec(std::string command); // throw(SshException);
    /** Run remote command and get output.*/
    [[nodiscard]] std::tuple<int /*status*/, std::shared_ptr<char*> /*output*/, std::string /*log*/> 
      exec_get_output(std::string command);// throw(SshException);
    /** Run remote command. stdin_string in a string that will be write in stdin of command.
     * @return status get status of output command and log info.*/
    [[nodiscard]] std::tuple<int /*status*/, std::string /*log*/> 
      exec(std::string command, std::string stdin_string); // throw(SshException);
    /** Run remote command and get output. stdin_string in a string that will be write in stdin of command.*/
    [[nodiscard]] std::tuple<int /*status*/, std::shared_ptr<char*> /*output*/, std::string /*log*/> 
      exec_get_output(std::string command, std::string stdin_string);// throw(SshException);
    /** Run remote command as sudo. if status == -1, user is not a sudoer.*/
    [[nodiscard]] std::tuple<int /*status*/, std::string /*log*/> 
      exec_sudo(std::string command);// throw(SshException);
    /** Run remote command as sudo and get output. if status == -1, user is not a sudoer.*/
    [[nodiscard]] std::tuple<int /*status*/, std::shared_ptr<char*> /*output*/, std::string /*log*/> 
      exec_sudo_get_output(std::string command);// throw(SshException);
    void scp_write(std::string filepath, std::string dest);// throw(SshException);
    void ssh_write_to_file(std::string content, std::string dest);// throw(SshException);


  private:
    [[nodiscard]] std::tuple<int /*status*/, std::shared_ptr<char*> /*output*/, std::string /*log*/> exec_sudo_get_output(std::string command, bool sudo, bool output_to_stdout, std::string stdin_string = "");

    ssh_session session;
    std::string host;
    int port;
    int verbosity;
    bool connected;
    std::string user, password;
};


class SshException : public std::exception
{
  public:
    SshException(std::string error);
    virtual const char *what();
  private:
    std::string error;
};

#endif
