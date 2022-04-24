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

#include "sshptr.h"
#include "configfileparser.h"
#include "simpleexception.h"
#include "manager.h"
#include <cstring>
#include <unistd.h>


void print_help(const char *command)
{
  std::cout << command << " scripts_file [--stdin] [--help]" << std::endl;
  std::cout << R"(
This command run a SSH session over a computer network. You can control a big set of terminals and run scripts on then.

The available options are:
--stdin               Read password from stdin.
--password password   Sets password
--no-multi            SSH scripts are run one by one, no multi-process.
--log_path path       Log files will be saved on "path". The default path is ".".

)";
}


int main(int argn, char* argv[])
{
  /*
  std::string host("localhost");
  std::shared_ptr<SshPtr> ssh = std::make_shared<SshPtr>(host, 22);
  bool ok = ssh->connect(std::string("testuser"), std::string("test"));
  if(ok) {
    try {
      int status;
      status = ssh->exec_sudo(std::string("ls -l /hshkdfjks"));
      std::shared_ptr<char *> output;
      std::tie(status, output) = ssh->exec_get_output(std::string("ls -l"));
      printf("\n>>>>%s<<<\n", *output);
      status = ssh->exec("ls /sfgsdfsfds"); 
      printf("status %d\n", status);
      std::tie(status, output) = ssh->exec_sudo_get_output(std::string("ls -l"));
      printf("\n>>>>%s<<<\n", *output);
      ssh->scp_write(std::string("/home/lucas/sdcard/prog/libssh/pruebas/Makefile"), std::string("/home/lucas/Descargas/Makefile.txt"));
    } catch(SshException &err) {
      fprintf(stderr, "Error: %s\n", err.what());
    }
  }
  */
  /*
  try {
  std::shared_ptr<ConfigItemVector> vec = ConfigFileParser::parser("script.txt");
  ConfigFileParser::print_tree(std::cout, vec); 
  } catch(SimpleException &error) {
    std::cerr << error.what() << std::endl;
  }*/
  std::string password;
  std::string scripts_file;
  std::string log_path;
  bool no_multi = false;

  for(int i = 0; i < argn; i++) {
    if(!strcmp(argv[i], "--help") || argn == 1) {
      print_help(argv[0]);
    } else if(argv[i][0] != '\0' && argv[i][0] != '-') {
      scripts_file = argv[i];
    } else if(!strcmp(argv[i], "--stdin")) {
      std::cin >> password;
    } else if(!strcmp(argv[i], "--no-multi")) {
      no_multi = true;
    } else if(!strcmp(argv[i], "--password")) {
      if(++i < argn)
        password = argv[i];
      else {
        std::cerr << "Error: --log_path needs path" << std::endl;
        print_help(argv[0]);
      }
    } else if(!strcmp(argv[i], "--log_path")) {
      if(++i < argn) {
        log_path = argv[i];
      } else {
        std::cerr << "Error: --log_path needs path" << std::endl;
        print_help(argv[0]);
      }
    }else {
      std::cerr << "Unknown argument: " << argv[i] << std::endl;
      print_help(argv[0]);
      return 1;
    }
  }

  if(scripts_file.empty()) {
    std::cerr << "No scripts file has been set." << std::endl;
    print_help(argv[0]);
    return 2;
  }

  if(password.empty()) {
    char *p = getpass("Password: ");
    if(p == NULL) {
      std::cerr << "No password has been read." << std::endl;
      return 4;
    }
    password = p;
  }

  int return_state = 0;
  // Uncomment next line to SSH debug output
  //ssh_set_log_level(SSH_LOG_PACKET);
  ssh_init();
  try {
    const std::set<std::string> tags = {"hosts", "scripts", "user", "host", "password", "script", "name", "command", "sudo", "stop_on_error", "args", "orig", "dest", "md5", "threads", "scripts_lock", "type", "upload", "download", "monitor"};
    std::shared_ptr<ConfigItemVector> scripts_and_host = ConfigFileParser::parser(scripts_file, tags);
    ConfigFileParser::print_tree(std::cout, scripts_and_host); 
    std::filesystem::path log_folder_path(log_path);
    
    Manager manager(scripts_and_host, password, no_multi, log_folder_path);
    manager.checkKeys();
    manager.run();
  } catch(SshException &error) {
    std::cerr << error.what() << std::endl;
    return_state = 3;
  } catch(SimpleException &error) {
    std::cerr << error.what() << std::endl;
    return_state = 3;
  } catch(std::exception &error) {
    std::cerr << error.what() << std::endl;
    return_state = 3;
  }

  std::cout << " *** Finish ***" << std::endl;
  ssh_finalize();
  return return_state;
}
