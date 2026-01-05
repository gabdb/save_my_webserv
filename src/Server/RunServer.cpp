#include "../../inc/temp.hpp"
#include <poll.h>

void TCPserver::run()
{
    static const int POLL_TIMEOUT_MS = 1000;  // 1s tick
    static const int CLIENT_TIMEOUT_S = 30;  // 30s idle

    while (true)
    {
        std::vector<struct pollfd> pfds;
        init_pollfds(pfds);
        if (pfds.empty())
            continue;

        int ready = poll(&pfds[0], pfds.size(), POLL_TIMEOUT_MS);
        if (ready < 0)
            throw std::runtime_error("poll() failed");

        // ---- timeout sweep (même si ready == 0) ----
        time_t now = time(NULL);
        for (std::map<int, Client>::iterator it = _clients.begin(); it != _clients.end(); )
        {
            int fd = it->first;
            Client &cl = it->second;

            if (now - cl.lastActivity > CLIENT_TIMEOUT_S)
            {
                ++it;                 // avancer avant erase
                closeClientConnexion(fd);
                continue;
            }
            ++it;
        }

        // ---- accept new clients ----
        for (size_t i = 0; i < _listeners.size(); ++i)
        {
            if (pfds[i].revents & POLLIN)
                acceptNewClient(pfds[i].fd);
        }

        // ---- handle client sockets + CGI pipes ----
        for (size_t i = _listeners.size(); i < pfds.size(); ++i)
        {
            int fd = pfds[i].fd;
            short revents = pfds[i].revents;

            if (revents == 0)
                continue;

            // 1) client socket ?
            std::map<int, Client>::iterator it = _clients.find(fd);
            if (it != _clients.end())
            {
                Client &client = it->second;

                if (client.state == CLOSE_CONNECTION) {
                    closeClientConnexion(fd);
                    continue;
                }

                if (revents & (POLLERR | POLLHUP | POLLNVAL)) {
                    closeClientConnexion(fd);
                    continue;
                }

                if ((revents & POLLIN) && client.wantRead)
                    ReadfromClient(client);

                if (client.state == HANDLE_REQUEST)
                    handleRequest(client);

                if ((revents & POLLOUT) && client.wantWrite)
                    WritetoClient(client);

                if (client.state == CLOSE_CONNECTION)
                    closeClientConnexion(fd);

                continue;
            }

            // 2) sinon: CGI pipe fd ?
            CGI *cgi = findCgiByFd(fd);
            if (cgi)
            {
                Client &cl = cgi->getClient();
                cl.lastActivity = now;

                // Ne traite PAS POLLHUP comme fatal ici:
                // sur stdout CGI, POLLHUP peut apparaître quand le child ferme le pipe,
                // mais on veut encore lire les derniers bytes.
                if (revents & (POLLERR | POLLNVAL))
                {
                    cleanupCgiForClient(cl);
                    generateErrorResponse(cl, 500);
                    continue;
                }

                handleCgiIo(fd, revents);

                if (cgi->isFinished())
                {
                    bool err = cgi->hasError() || (cgi->getExitStatus() != 0);

                    std::string out;
                    try {
                        out = getCgiOutputForClient(cl); // delete + erase CGI
                    } catch (...) {
                        generateErrorResponse(cl, 500);
                        continue;
                    }

                    if (err) generateErrorResponse(cl, 500);
                    else     buildAndSendCgiResponse(cl, out);
                }
                continue;
            }

            // 3) fd inconnu: ignore
        }
    }
}

void TCPserver::init_pollfds(std::vector<struct pollfd> &pfds) {
  pfds.reserve(_listeners.size() + _clients.size() + _cgis.size() * 2);

  // Listening sockets (server side)
  for (size_t i = 0; i < _listeners.size(); ++i) {
    struct pollfd p;
    p.fd = _listeners[i].fd;
    p.events = POLLIN;
    p.revents = 0;
    pfds.push_back(p);
  }

  // Client sockets (browser <-> server TCP connections)
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

  // CGI pipes (stdin/stdout)
  for (std::map<int, CGI *>::iterator it = _cgis.begin(); it != _cgis.end(); ++it) {
    it->second->registerPollFds(pfds);
  }
  
}

void TCPserver::closeClientConnexion(int fd) {
  std::map<int, Client>::iterator it = _clients.find(fd);
  if (it != _clients.end()) {
    cleanupCgiForClient(it->second);
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
    c.keepAlive = false; //p-e pas nécessaire

    // rajout gab
    c.locationBlock = NULL; // update apres parsing requete
    c.state = READ_REQUEST; // 1e stade Enum
    c.bodyExpected = 0;     // stock la valeur de Content-Length
    c.bodyReceived = 0;     // mise à jour au fur et à mesure des recv()
    c.request.bodyComplete = false;
    c.request.headersComplete = false;
    c.listenerFd = listenFd;
    c.lastActivity = time(NULL);
    _clients[clientFd] = c;
  }
}
