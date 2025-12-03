/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CGI.cpp                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: webserv-team <webserv@student.42.fr>       +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/27 00:00:00 by webserv-team     #+#    #+#             */
/*   Updated: 2025/11/27 00:00:00 by webserv-team    ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../inc/temp.hpp"

#include <cctype>
#include <cerrno>
#include <csignal>
#include <cstring>
#include <stdexcept>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

static void setNonBlocking(int fd) {
  if (fd < 0)
    return;
  int flags = fcntl(fd, F_GETFL, 0);
  if (flags < 0)
    return; // if this fails we just keep the default flags
  fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

Client &CGI::getClient() const {
  return *this->_client;
}

const std::string &CGI::getRawOutput() const {
  return this->_fromCgi;
}

int CGI::getStdoutFd() const {
  return _stdoutFd;
}

bool CGI::isFinished() const {
  return this->_finished;
}

std::vector<std::string> buildCgiEnv(const Client &client, const std::string &scriptPath, const std::string &queryString) {
  std::vector<std::string> env;
  const HttpRequest &req = client.request;
  std::map<std::string, std::string>::const_iterator it;

  env.push_back("GATEWAY_INTERFACE=CGI/1.1");
  env.push_back(std::string("REQUEST_METHOD=") + req.method);
  env.push_back(std::string("SCRIPT_FILENAME=") + scriptPath);
  env.push_back(std::string("SCRIPT_NAME=") + scriptPath);
  env.push_back(std::string("QUERY_STRING=") + queryString);
  env.push_back(std::string("SERVER_PROTOCOL=") + req.version);
  // TODO:
  //  env.push_back(std::string("SERVER_SOFTWARE="));
  //  env.push_back(std::string("SERVER_NAME="));
  //  env.push_back(std::string("SERVER_PORT="));
  //  env.push_back(std::string("PATH_INFO="));

  it = req.headers.find("Content-Length");
  if (it != req.headers.end())
    env.push_back(std::string("CONTENT_LENGTH=") + it->second);

  it = req.headers.find("Content-Type");
  if (it != req.headers.end())
    env.push_back(std::string("CONTENT_TYPE=") + it->second);

  for (std::map<std::string, std::string>::const_iterator hit = req.headers.begin();
       hit != req.headers.end(); ++hit) {
    std::string key = hit->first;
    std::string value = hit->second;

    // Convert header name to upper case and replace '-' by '_'.
    for (size_t i = 0; i < key.size(); ++i) {
      if (key[i] == '-')
        key[i] = '_';
      else
        key[i] = static_cast<char>(std::toupper(static_cast<unsigned char>(key[i])));
    }

    env.push_back(std::string("HTTP_") + key + "=" + value);
  }

  return env;
}

// =====================
//       CGI methods
// =====================

CGI::CGI(Client &client,
         const std::string &interpreterPath,
         const std::string &scriptPath,
         const std::string &queryString)
    : _client(&client), _pid(-1), _stdinFd(-1), _stdoutFd(-1),
      _writingBody(false), _readingOutput(true), _finished(false),
      _error(false), _exitStatus(0) {
  startProcess(interpreterPath, scriptPath, queryString);

  if (!client.request.body.empty()) {
    _toCgi = client.request.body;
    _writingBody = true;
  }
}

CGI::~CGI() {
  closePipes();
}

void CGI::startProcess(const std::string &interpreterPath,
                       const std::string &scriptPath,
                       const std::string &queryString) {
  int stdinPipe[2];  // [0] read (CGI stdin), [1] write (server writes)
  int stdoutPipe[2]; // [0] read (server reads), [1] write (CGI stdout)

  if (pipe(stdinPipe) < 0 || pipe(stdoutPipe) < 0) {
    throw std::runtime_error("Failed to create CGI pipes");
    closePipes();
  }

  pid_t pid = fork();
  if (pid < 0) {
    close(stdinPipe[0]);
    close(stdinPipe[1]);
    close(stdoutPipe[0]);
    close(stdoutPipe[1]);
    throw std::runtime_error("fork() failed for CGI");
  }

  if (pid == 0) {
    // ========================
    //      Child process
    // ========================

    dup2(stdinPipe[0], STDIN_FILENO);
    dup2(stdoutPipe[1], STDOUT_FILENO);

    close(stdinPipe[0]);
    close(stdinPipe[1]);
    close(stdoutPipe[0]);
    close(stdoutPipe[1]);

    std::vector<char *> argv;
    argv.push_back(const_cast<char *>(interpreterPath.c_str()));
    argv.push_back(const_cast<char *>(scriptPath.c_str()));
    argv.push_back(0);

    std::vector<std::string> envStrings = buildCgiEnv(*_client, scriptPath, queryString);
    std::vector<char *> envp;
    for (size_t i = 0; i < envStrings.size(); ++i)
      envp.push_back(const_cast<char *>(envStrings[i].c_str()));
    envp.push_back(0);

    std::string::size_type lastSlash = scriptPath.rfind('/');
    if (lastSlash != std::string::npos) {
      std::string dir = scriptPath.substr(0, lastSlash);
      if (!dir.empty())
        chdir(dir.c_str());
    }

    execve(interpreterPath.c_str(), &argv[0], &envp[0]);
    _exit(1); // execve failed
  }

  // ========================
  //     Parent process
  // ========================
  this->_pid = pid;

  close(stdinPipe[0]);
  close(stdoutPipe[1]);

  setNonBlocking(stdinPipe[1]);
  setNonBlocking(stdoutPipe[0]);

  this->_stdinFd = stdinPipe[1];
  this->_stdoutFd = stdoutPipe[0];
}

void CGI::registerPollFds(std::vector<struct pollfd> &pfds) const {
  if (_writingBody && _stdinFd >= 0 && !_toCgi.empty()) {
    struct pollfd p;
    p.fd = _stdinFd;
    p.events = POLLOUT;
    p.revents = 0;
    pfds.push_back(p);
  }
  if (_readingOutput && _stdoutFd >= 0) {
    struct pollfd p;
    p.fd = _stdoutFd;
    p.events = POLLIN;
    p.revents = 0;
    pfds.push_back(p);
  }
}

bool CGI::ownsFd(int fd) const {
  return (fd == _stdinFd || fd == _stdoutFd);
}

void CGI::handleIo(int fd, short revents) {
  writeBody(fd, revents);
  readOutput(fd, revents);
}

void CGI::writeBody(int fd, short revents) {
  if (!(revents & POLLOUT))
    return;
  if (!_writingBody || _stdinFd < 0 || fd != _stdinFd)
    return;

  if (_toCgi.empty()) {
    close(_stdinFd);
    _stdinFd = -1;
    _writingBody = false;
    return;
  }

  ssize_t written = write(_stdinFd, _toCgi.data(), _toCgi.size());
  if (written <= 0) {
    close(_stdinFd);
    _stdinFd = -1;
    _writingBody = false;
    return;
  }

  _toCgi.erase(0, static_cast<size_t>(written));
  if (_toCgi.empty()) {
    close(_stdinFd);
    _stdinFd = -1;
    _writingBody = false;
  }
}

void CGI::readOutput(int fd, short revents) {
  if (!(revents & POLLIN))
    return;
  if (_stdoutFd < 0 || fd != _stdoutFd)
    return;

  char buffer[4096];
  ssize_t bytes = read(_stdoutFd, buffer, sizeof(buffer));
  if (bytes > 0) {
    _fromCgi.append(buffer, static_cast<size_t>(bytes));
  }
  else {
    close(_stdoutFd);
    _stdoutFd = -1;
    _readingOutput = false;
    _finished = true;

    if (_pid > 0) {
      int status = 0;
      pid_t w = waitpid(_pid, &status, WNOHANG);
      if (w > 0)
        _exitStatus = status;
    }
  }
}

void CGI::closePipes() {
  if (_stdinFd >= 0) {
    close(_stdinFd);
    _stdinFd = -1;
  }
  if (_stdoutFd >= 0) {
    close(_stdoutFd);
    _stdoutFd = -1;
  }
  if (_pid > 0) {
    int status = 0;
    pid_t w = waitpid(_pid, &status, WNOHANG);
    if (w > 0)
      _exitStatus = status;
    _pid = -1;
  }
}

// =============================
//  TCPserver CGI helper methods
// =============================

void TCPserver::startCgiForClient(Client &client,
                                  const std::string &interpreterPath,
                                  const std::string &scriptPath,
                                  const std::string &queryString) {
  // ensure no running CGI for this client
  for (std::map<int, CGI *>::iterator it = _cgis.begin(); it != _cgis.end(); ++it) {
    if (&it->second->getClient() == &client && !it->second->isFinished())
      throw std::runtime_error("CGI already running for this client");
  }

  CGI *cgi = new CGI(client, interpreterPath, scriptPath, queryString);
  _cgis.insert(std::make_pair(cgi->getStdoutFd(), cgi));
}

bool TCPserver::isCgiFinishedForClient(const Client &client) const {
  for (std::map<int, CGI *>::const_iterator it = _cgis.begin(); it != _cgis.end(); ++it) {
    if (&it->second->getClient() == &client && it->second->isFinished())
      return true;
  }
  return false;
}

std::string TCPserver::getCgiOutputForClient(const Client &client) {
  for (std::map<int, CGI *>::iterator it = _cgis.begin(); it != _cgis.end(); ++it) {
    if (&it->second->getClient() == &client && it->second->isFinished()) {
      std::string result = it->second->getRawOutput();
      delete it->second;
      _cgis.erase(it);
      return result;
    }
  }
  throw std::runtime_error("No finished CGI for this client");
}

CGI *TCPserver::findCgiByFd(int fd) {
  for (std::map<int, CGI *>::iterator it = _cgis.begin(); it != _cgis.end(); ++it) {
    if (it->second->ownsFd(fd))
      return it->second;
  }
  return 0;
}

// const CGI *TCPserver::findCgiByFd(int fd) const {
//   for (std::map<int, CGI *>::const_iterator it = _cgis.begin(); it != _cgis.end(); ++it) {
//     if (it->second->ownsFd(fd))
//       return it->second;
//   }
//   return 0;
// }

void TCPserver::cleanupCgiForClient(const Client &client) {
  for (std::map<int, CGI *>::iterator it = _cgis.begin(); it != _cgis.end();) {
    if (&it->second->getClient() == &client) {
      delete it->second;
      std::map<int, CGI *>::iterator toErase = it++;
      _cgis.erase(toErase);
    }
    else {
      ++it;
    }
  }
}

void TCPserver::handleCgiIo(int fd, short revents) {
  CGI *cgi = findCgiByFd(fd);
  if (!cgi)
    return;
  cgi->handleIo(fd, revents);
}
