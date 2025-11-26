#include "../../inc/temp.hpp"
#include <poll.h>

void TCPserver::run() {
  while (true) {
    std::vector<struct pollfd> pfds;
    init_pollfds(pfds);
    int ready = poll(&pfds[0], pfds.size(), -1);
    if (ready < 0)
      throw std::runtime_error("poll() failed");
    // if ready == 0: timeout reached
    //  if (ready == 0)
    //    std::cout << "timed out, restarting loop" << std::endl;
    //    continue;
    for (size_t i = 0; i < _listeners.size(); ++i) {
      if (pfds[i].revents & POLLIN)
        acceptNewClient(pfds[i].fd);
    }
    for (size_t i = _listeners.size(); i < pfds.size(); ++i) {
      int fd = pfds[i].fd;
      std::map<int, Client>::iterator it = _clients.find(fd);
      if (it == _clients.end())
        continue; // if pfds[i].fd not found in clients, jump to next iteration of i

      // Check for end of client life, if EOL: clean up
      if (pfds[i].revents & (POLLERR | POLLHUP | POLLNVAL)) {
        closeClientConnexion(fd);
        continue;
      }
      // Client &client = it->second;
      // if (pfds[i].revents & POLLIN)
      //   ReadfromClient(client); // Gab TODO: Start implementing recv logic here
      // if (pfds[i].revents & POLLOUT)
      //   WritetoClient(client); // Gab TODO: Start implementing send logic here
    }
  }
}

void TCPserver::init_pollfds(std::vector<struct pollfd> &pfds) {
  // add _listener fds to poll's fds to read from (POLLIN)
  pfds.reserve(_listeners.size() + _clients.size());
  for (size_t i = 0; i < _listeners.size(); ++i) {
    struct pollfd p;
    p.fd = _listeners[i].fd;
    p.events = POLLIN;
    p.revents = 0;
    pfds.push_back(p);
  }

  // add _client fds to poll's fds to read from/write to (POLLIN/POLLOUT)
  for (std::map<int, Client>::iterator it = _clients.begin(); it != _clients.end(); ++it) {
    struct pollfd p;
    p.fd = it->first;
    p.events = 0;
    if (it->second.wantRead)
      p.events |= POLLIN; // |= is "OR" bitwise op.
    if (it->second.wantWrite && !it->second.sendBuffer.empty())
      p.events |= POLLOUT;
    p.revents = 0;
    pfds.push_back(p);
  }
}

void TCPserver::closeClientConnexion(int fd) {
  std::map<int, Client>::iterator it = _clients.find(fd);
  if (it != _clients.end()) {
    ::close(fd);
    _clients.erase(it);
  }
}

void TCPserver::acceptNewClient(int listenFd) {
  while (true) {
    int clientFd = accept(listenFd, NULL, NULL);
    if (clientFd < 0)
      break;
    if (fcntl(clientFd, F_SETFL, O_NONBLOCK) < 0) {
      ::close(clientFd);
      continue;
    }

    Client c;
    c.fd = clientFd;
    c.recvBuffer.clear();
    c.sendBuffer.clear();
    c.wantRead = true;
    c.wantWrite = false;
    c.serverBlock = NULL; // TODO: find which Listener (server) this belongs to
	c.keepAlive = false;

	//rajout gab
	c.locationBlock = NULL; //update apres parsing requete
	c.state = READ_REQUEST; //1e stade Enum
	c.bodyExpected = 0; //stock la valeur de Content-Length
	c.bodyReceived = 0; //mise à jour au fur et à mesure des recv()
	c.request.bodyComplete = false;
	c.request.headersComplete = false;

    _clients[clientFd] = c;
  }
}
