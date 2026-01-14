#include "../../inc/temp.hpp"
#include <poll.h>

void TCPserver::run() {
    static const int POLL_TIMEOUT_MS = 1000;

    while (true) {
        std::vector<struct pollfd> pfds;
        init_pollfds(pfds);
        if (pfds.empty())
            continue;

        int ready = poll(&pfds[0], pfds.size(), POLL_TIMEOUT_MS);
        if (ready < 0)
            throw std::runtime_error("poll() failed");

        for (size_t i = 0; i < _listeners.size(); ++i) {
            if (pfds[i].revents & POLLIN)
                acceptNewClient(pfds[i].fd);
        }

        for (size_t i = _listeners.size(); i < pfds.size(); ++i) {
            int fd = pfds[i].fd;
            short revents = pfds[i].revents;

            handleClientIO(fd, revents);
            handleCgiIO(fd, revents);
        }
    }
}

void TCPserver::handleClientIO(int fd, short revents) {
    static const int CLIENT_TIMEOUT_S = 30; // 30s idle
    time_t now = time(NULL);
    std::map<int, Client>::iterator it = _clients.find(fd);

    if (it != _clients.end()) {
        Client &client = it->second;

        if (client.state == CLOSE_CONNECTION ||
            now - client.lastActivity > CLIENT_TIMEOUT_S ||
            revents & (POLLERR | POLLHUP | POLLNVAL))
            closeClientConnexion(fd);

        if (revents == 0)
            return;

        if (client.state == READ_REQUEST && (revents & POLLIN))
            ReadfromClient(client);

        if (client.state == HANDLE_REQUEST)
            handleRequest(client);

        if (client.state == SEND_RESPONSE && (revents & POLLOUT))
            WritetoClient(client);
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
    for (std::map<int, Client>::iterator it = _clients.begin();
         it != _clients.end(); ++it) {
        struct pollfd p;
        p.fd = it->first;
        p.events = 0;
        if (it->second.state == READ_REQUEST)
            p.events |= POLLIN; // |= is "OR" bitwise op.
        if (it->second.state == SEND_RESPONSE && !it->second.sendBuffer.empty())
            p.events |= POLLOUT;
        p.revents = 0;
        pfds.push_back(p);
    }

    // CGI pipes (stdin/stdout)
    for (std::map<int, CGI *>::iterator it = _cgis.begin(); it != _cgis.end();
         ++it) {
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
        c.serverBlock = NULL;

        // rajout gab
        c.locationBlock = NULL;
        c.state = READ_REQUEST;
        c.bodyExpected = 0;
        c.bodyReceived = 0;
        c.chunked = false;
        c.request.bodyComplete = false;
        c.request.headersComplete = false;
        c.listenerFd = listenFd;
        c.lastActivity = time(NULL);
        _clients[clientFd] = c;
    }
}
