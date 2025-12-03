#ifndef CGI_HPP
#define CGI_HPP

// #include "TCPserver.hpp"
#include <string>
#include <vector>

class Client;

class CGI {
public:
  CGI(Client &client,
      const std::string &interpreterPath,
      const std::string &scriptPath,
      const std::string &queryString);

  CGI(const CGI &);
  CGI &operator=(const CGI &);
  ~CGI();

  void registerPollFds(std::vector<struct pollfd> &pfds) const;
  Client &getClient() const;
  const std::string &getRawOutput() const;
  int getStdoutFd() const;
  bool isFinished() const;
  bool ownsFd(int fd) const;
  void handleIo(int fd, short revents);
  void closePipes();

private:
  Client *_client;
  pid_t _pid;
  int _stdinFd;
  int _stdoutFd;
  bool _writingBody;
  bool _readingOutput;
  bool _finished;
  bool _error;
  int _exitStatus;
  std::string _toCgi;
  std::string _fromCgi;

  void startProcess(const std::string &interpreterPath,
                    const std::string &scriptPath,
                    const std::string &queryString);
  void writeBody(int fd, short revents);
  void readOutput(int fd, short revents);
};

#endif
