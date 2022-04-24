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
#include "string_utils.h"
#include <errno.h>
#include <string.h>
#include <filesystem>
#include <sys/stat.h>
#include <iostream>

SshException::SshException(std::string error)
{
  this->error = error;
  std::cerr << "SshException: " << error << std::endl;
}

const char *SshException::what() 
{
  return error.c_str();
}

static int verify_knownhost(ssh_session session)
{
    enum ssh_known_hosts_e state;
    unsigned char *hash = NULL;
    ssh_key srv_pubkey = NULL;
    size_t hlen;
    char *hexa;
    int rc;
 
    rc = ssh_get_server_publickey(session, &srv_pubkey);
    if (rc < 0) {
        return -1;
    }
 
    rc = ssh_get_publickey_hash(srv_pubkey,
                                SSH_PUBLICKEY_HASH_SHA1,
                                &hash,
                                &hlen);
    ssh_key_free(srv_pubkey);
    if (rc < 0) {
        return -1;
    }
 
    state = ssh_session_is_known_server(session);
    switch (state) {
        case SSH_KNOWN_HOSTS_OK:
            /* OK */
 
            break;
        case SSH_KNOWN_HOSTS_CHANGED:
            fprintf(stderr, "Host key for server changed: it is now:\n");
            ssh_print_hash(SSH_PUBLICKEY_HASH_SHA1, hash, hlen);
            fprintf(stderr, "For security reasons, connection will be stopped\n");
            ssh_clean_pubkey_hash(&hash);
 
            return -1;
        case SSH_KNOWN_HOSTS_OTHER:
            fprintf(stderr, "The host key for this server was not found but an other"
                    "type of key exists.\n");
            fprintf(stderr, "An attacker might change the default server key to"
                    "confuse your client into thinking the key does not exist\n");
            ssh_clean_pubkey_hash(&hash);
 
            return -1;
        case SSH_KNOWN_HOSTS_NOT_FOUND:
            fprintf(stderr, "Could not find known host file.\n");
            fprintf(stderr, "If you accept the host key here, the file will be"
                    "automatically created.\n");
 
            /* FALL THROUGH to SSH_SERVER_NOT_KNOWN behavior */
 
        case SSH_KNOWN_HOSTS_UNKNOWN:
            hexa = ssh_get_hexa(hash, hlen);
            fprintf(stderr,"The server is unknown. Do you trust the host key?\n");
            fprintf(stderr, "Public key hash: %s\n", hexa);
            ssh_string_free_char(hexa);
            ssh_clean_pubkey_hash(&hash);
 
            rc = ssh_session_update_known_hosts(session);
            if (rc < 0) {
                fprintf(stderr, "Error %s\n", strerror(errno));
                return -1;
            }
 
            break;
        case SSH_KNOWN_HOSTS_ERROR:
            fprintf(stderr, "Error %s", ssh_get_error(session));
            ssh_clean_pubkey_hash(&hash);
            return -1;
    }
 
    ssh_clean_pubkey_hash(&hash);
    return 0;
}


SshPtr::SshPtr(std::string host, int port) {
  connected = false;
  session = ssh_new();
  this->port = port;
  this->host = host;
  verbosity = SSH_LOG_WARNING;

  if(session == nullptr) {
    fprintf(stderr, "Error: libssh couldn't be init.");
    exit(1);
  }
  ssh_options_set(session, SSH_OPTIONS_HOST, this->host.c_str());
  ssh_options_set(session, SSH_OPTIONS_LOG_VERBOSITY, &verbosity);
  ssh_options_set(session, SSH_OPTIONS_PORT, &(this->port));
}


SshPtr::~SshPtr() {
  if(connected)
    ssh_disconnect(session);
  ssh_free(session);
  printf("SSH session to %s finished.\n", host.c_str());
}


[[nodiscard]] bool SshPtr::connect(std::string user, std::string password) {
  int rc = ssh_connect(session);
  connected = rc == SSH_OK;
  if(!connected) {
    fprintf(stderr, "Error connecting to host: %s\n", ssh_get_error(session));
    return false;
  }
  if (verify_knownhost(session) < 0) {
    ssh_disconnect(session);
    connected = false;
    return connected;
  }
  rc = ssh_userauth_password(session, user.c_str(), password.c_str());
  if (rc != SSH_AUTH_SUCCESS) {
    fprintf(stderr, "Error authenticating with password: %s\n", ssh_get_error(session));
    ssh_disconnect(session);
    connected = false;
    return connected;  
  }
  if(connected) {
    this->user = user;
    this->password = password;
  }
  return connected;
}

enum LogState {
  NONE, LOG_SHARP1, LOG_SHARP2, LOG_L, LOG_O, LOG_G, LOG_COLON
};

[[nodiscard]] std::tuple<int /*status*/, std::shared_ptr<char*> /*output*/, std::string /*log*/> SshPtr::exec_sudo_get_output(std::string command, bool sudo, bool output_to_stdout, std::string stdin_string)
{
  ssh_channel channel;
  int rc;
  char buffer[1024];
  int nbytes, len = 1;
  char *output = (char *) malloc(sizeof(char));
  *output = '\0';
  std::string log;
  LogState state = LogState::NONE;
 
  channel = ssh_channel_new(session);
  if (channel == NULL)
    throw(SshException(std::string("Error: Channel cannot be opened.")));
 
  rc = ssh_channel_open_session(channel);
  if (rc != SSH_OK) {
    ssh_channel_free(channel);
    throw(SshException(std::string("Error: Channel cannot be opened.")));
  }
 
  if(sudo)
    command = "sudo -Sp '' " + command;

  std::cout << "\033[34m" << user << "@" << host << ": \033[1;32m" << command << "\033[0m" << std::endl;
  rc = ssh_channel_request_exec(channel, command.c_str());
  if (rc != SSH_OK) {
    ssh_channel_close(channel);
    ssh_channel_free(channel);
    throw(SshException(std::string("Error: Output from command '") + command + "' cannot be run."));
  }

  if(sudo) {
    std::string pass = password + "\n";
    ssh_channel_write(channel, pass.c_str(), strlen(pass.c_str()));
  }
  
  if(! stdin_string.empty()) {
    ssh_channel_write(channel, stdin_string.c_str(), strlen(stdin_string.c_str()));
  }

  nbytes = ssh_channel_read_timeout(channel, buffer, sizeof(buffer), 0, -1);
  while (nbytes > 0) {
    // Read log
    for(int i = 0; i < nbytes; i++) {
      char ch = buffer[i];
      switch(state) {
        case LogState::NONE:
          if(ch == '#')
            state = LogState::LOG_SHARP1;
          break;
        case LogState::LOG_SHARP1:
          if(ch == '#')
            state = LogState::LOG_SHARP2;
          else
            state = LogState::NONE;
          break;
        case LogState::LOG_SHARP2:
          if(ch == 'l')
            state = LogState::LOG_L;
          else
            state = LogState::NONE;
          break;
        case LogState::LOG_L:
          if(ch == 'o')
            state = LogState::LOG_O;
          else
            state = LogState::NONE;
          break;
        case LogState::LOG_O:
          if(ch == 'g')
            state = LogState::LOG_G;
          else
            state = LogState::NONE;
          break;
        case LogState::LOG_G:
          if(ch == ':')
            state = LogState::LOG_COLON;
          else
            state = LogState::NONE;
          break;
        case LogState::LOG_COLON:
          log += ch;
          if(ch == '\n')
            state = LogState::NONE;
          break;
      }
    }
    // Write output to stdout or output
    len += nbytes;
    if(output_to_stdout) {
      if (write(1, buffer, nbytes) != (unsigned int) nbytes) {
        ssh_channel_close(channel);
        ssh_channel_free(channel);
        throw(SshException(std::string("Error: Output from command '") + command + "' cannot be read. 1"));
      }
    } else {
      output = (char *) realloc(output, len);
      if(output == NULL) {
        ssh_channel_close(channel);
        ssh_channel_free(channel);
        throw(SshException(std::string("Error: Output from command '") + command + "' cannot be read. 2"));
      } else {
        strncat(output, buffer, nbytes);
      }
    }
    if(!ssh_channel_is_eof(channel) || !ssh_channel_is_closed(channel))
      nbytes = ssh_channel_read_timeout(channel, buffer, sizeof(buffer), 0, -1);
    else
      nbytes = 0;
  }
 
  //if(nbytes < 0)
  //{
  //  ssh_channel_close(channel);
  //  ssh_channel_free(channel);
  //  throw(SshException(std::string("Error: Output from command '") + command + "' cannot be read. 3"));
  //}
 
  ssh_channel_send_eof(channel);
  ssh_channel_close(channel);
  int status = ssh_channel_get_exit_status(channel);
  ssh_channel_free(channel);
  std::shared_ptr<char*> out = std::make_shared<char*>(output);
  return std::make_tuple(status, out, log);
}

[[nodiscard]] std::tuple<int /*status*/, std::string /*log*/> SshPtr::exec(std::string command) // throw(SshException);
{
  int status;
  std::shared_ptr<char*> output;
  std::string log;
  std::tie(status, output, log) = exec_sudo_get_output(command, false, true);
  return std::make_tuple(status, log);
}


[[nodiscard]] std::tuple<int /*status*/, std::shared_ptr<char*> /*output*/, std::string /*log*/> SshPtr::exec_get_output(std::string command) // throw(SshException);
{
  return exec_sudo_get_output(command, false, false);
}

[[nodiscard]] std::tuple<int /*status*/, std::string /*log*/> SshPtr::exec(std::string command, std::string stdin_string) // throw(SshException);
{
  int status;
  std::shared_ptr<char*> output;
  std::string log;
  std::tie(status, output, log) = exec_sudo_get_output(command, false, true, stdin_string);
  return std::make_tuple(status, log);
}

[[nodiscard]] std::tuple<int /*status*/, std::shared_ptr<char*> /*output*/, std::string /*log*/> SshPtr::exec_get_output(std::string command, std::string stdin_string) // throw(SshException);
{
  return exec_sudo_get_output(command, false, false, stdin_string);
}

[[nodiscard]] std::tuple<int /*status*/, std::string /*log*/> SshPtr::exec_sudo(std::string command) // throw(SshException);
{
  int status;
  std::shared_ptr<char*> output;
  std::string log;
  // Check if user is a sudoers
  std::tie(status, output, log) = exec_sudo_get_output("echo Ok", true, false);
  log = *output;
  if(strip(log) == "Ok") // User is a sudoer, then exec command
    std::tie(status, output, log) = exec_sudo_get_output(command, true, true);
  else { // User is not a sudoer
    status = -1;
    log = "Error: " + user + "@" + host + " is not in sudoers.";
  }
  return std::make_tuple(status, log);
}

[[nodiscard]] std::tuple<int /*status*/, std::shared_ptr<char*> /*output*/, std::string /*log*/> SshPtr::exec_sudo_get_output(std::string command) // throw(SshException);
{
  int status;
  std::shared_ptr<char*> output;
  std::string log;
  // Check if user is a sudoers
  std::tie(status, output, log) = exec_sudo_get_output("echo Ok", true, false);
  log = *output;
  if(strip(log) == "Ok") // User is a sudoer, then exec command
    std::tie(status, output, log) = exec_sudo_get_output(command, true, false);
  else { // User is not a sudoer
    status = -1;
    log = "Error: " + user + "@" + host + " is not in sudoers.";
  }
  return std::make_tuple(status, output, log);
}



void SshPtr::scp_write(std::string filepath, std::string dest)
{
  // Check file size
  std::filesystem::path path(filepath);
  if(! std::filesystem::exists(path)) {
    throw(SshException("[SshPtr::scp_write]: File " + filepath + " does not exists."));
  }

  std::error_code error;
  uintmax_t file_length = std::filesystem::file_size(path, error);
  if(error) {
    throw(SshException("[SshPtr::scp_write]: " + filepath + ":" + error.message()));
  }

  std::filesystem::path destPath(dest);
  // Make remote directory with path
  int rc;
  std::string log;
  std::tie(rc, log) = exec("mkdir -p '" + destPath.parent_path().string() + "'");
  if(rc != 0 && rc != 1) {
    throw(SshException("[SshPtr::scp_write]: Error making remote path: " + destPath.parent_path().string())); 
  }

  ssh_scp scp;
  scp = ssh_scp_new(session, SSH_SCP_WRITE, destPath.parent_path().string().c_str());
  if (scp == NULL) {
    throw(SshException("[SshPtr::scp_write]: Error allocating scp session: " + filepath + std::string(":") + ssh_get_error(session)));
  }
 
  rc = ssh_scp_init(scp);
  if (rc != SSH_OK) {
    ssh_scp_free(scp);
    throw(SshException("[SshPtr::scp_write]: Error initializing scp session: " + filepath + std::string(":") + ssh_get_error(session)));
  }
 
  rc = ssh_scp_push_file(scp, dest.c_str(), file_length, S_IRUSR | S_IWUSR | S_IRGRP);
  if (rc != SSH_OK)
    throw(SshException("[SshPtr::scp_write]: Cannot open remote file: " + dest + std::string(":") + ssh_get_error(session)));

  char buffer[4096];
  FILE *in = fopen(filepath.c_str(), "r");
  if(in == nullptr) {
    throw(SshException("[SshPtr::scp_write]: Cannot open local file: " + filepath));
  }

  size_t nbytes = fread(buffer, sizeof(char), sizeof(buffer), in);
  while(nbytes > 0) {
    rc = ssh_scp_write(scp, buffer, nbytes);
    if (rc != SSH_OK) {
      ssh_scp_close(scp);
      ssh_scp_free(scp);
      fclose(in);
      throw(SshException("[SshPtr::scp_write]: Cannot write to remote file: " + dest + std::string(":") + ssh_get_error(session)));
    }
    nbytes = fread(buffer, sizeof(char), sizeof(buffer), in);
  }

  fclose(in);
  ssh_scp_close(scp);
  ssh_scp_free(scp);
}

void SshPtr::ssh_write_to_file(std::string content, std::string dest) // throw(SshException);
{
  std::error_code error;

  std::filesystem::path destPath(dest);
  // Make remote directory with path
  int rc;
  std::string log;
  std::tie(rc, log) = exec("mkdir -p '" + destPath.parent_path().string() + "'");
  if(rc != 0 && rc != 1) {
    throw(SshException("[SshPtr::ssh_write_to_file]: Error making remote path: " + destPath.parent_path().string())); 
  }

  ssh_scp scp;
  scp = ssh_scp_new(session, SSH_SCP_WRITE, destPath.parent_path().string().c_str());
  if (scp == NULL) {
    throw(SshException("[SshPtr::ssh_write_to_file]: Error allocating scp session: " + dest + std::string(":") + ssh_get_error(session)));
  }
 
  rc = ssh_scp_init(scp);
  if (rc != SSH_OK) {
    ssh_scp_free(scp);
    throw(SshException("[SshPtr::scp_write]: Error initializing scp session: " + dest + std::string(":") + ssh_get_error(session)));
  }
 
  size_t nbytes = strlen(content.c_str());
  rc = ssh_scp_push_file(scp, dest.c_str(), nbytes, S_IRUSR | S_IWUSR | S_IRGRP);
  if (rc != SSH_OK)
    throw(SshException("[SshPtr::ssh_write_to_file]: Cannot open remote file: " + dest + std::string(":") + ssh_get_error(session)));

  rc = ssh_scp_write(scp, content.c_str(), nbytes);
  if (rc != SSH_OK) {
    ssh_scp_close(scp);
    ssh_scp_free(scp);
    throw(SshException("[SshPtr::ssh_write_to_file]: Cannot write to remote file: " + dest + std::string(":") + ssh_get_error(session)));
  }

  ssh_scp_close(scp);
  ssh_scp_free(scp);
}
