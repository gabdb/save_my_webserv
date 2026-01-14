/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   TCPserver.hpp                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gnyssens <gnyssens@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/13 13:56:07 by nicolive          #+#    #+#             */
/*   Updated: 2026/01/12 11:51:57 by gnyssens         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef __TCPSERVER_HPP__
#define __TCPSERVER_HPP__

#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <dirent.h>
#include <fcntl.h>
#include <fstream>
#include <map>
#include <netinet/in.h>
#include <poll.h>
#include <sstream>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "../config/Config.hpp"

class CGI;

// Server host:port entries
struct Listener {
    int fd;
    std::string host;
    int port;
    std::vector<const Block *> servers; // pointers to respective server blocks
                                        // who are using this listener
};

struct HttpRequest {
    std::string method;  // GET/POST/DELETE
    std::string target;  // URL demandée par le client
    std::string version; // souvent: HTTP/1.1

    std::map<std::string, std::string> headers;
    std::string body; // surtout utile pr POST

    bool headersComplete; // devient true quand "\r\n\r\n"
    bool bodyComplete;    // devient true si bodyReceived == bodyExpected
};

enum ClientState {
    READ_REQUEST,
    HANDLE_REQUEST,
    SEND_RESPONSE,
    CLOSE_CONNECTION
};

struct Client {
    int fd;
    std::string recvBuffer;
    std::string sendBuffer;
    const Block *serverBlock;

    const Block *locationBlock;

    HttpRequest request;
    ClientState state;
    size_t bodyExpected;
    size_t bodyReceived;
    bool chunked;
    size_t chunkBytesRemaining;
    int listenerFd;

    time_t lastActivity;
};

class TCPserver {
  public:
    TCPserver(const Config &cfg);
    ~TCPserver();

    const std::vector<Listener> &getListeners() const;

    void init();
    void run();

    void startCgiForClient(Client &client, const std::string &interpreterPath,
                           const std::string &scriptPath,
                           const std::string &queryString);
    std::string getCgiOutputForClient(const Client &client);

  private:
    const Config &_config;
    std::vector<Listener> _listeners;
    std::map<int, Client> _clients;
    std::map<int, CGI *> _cgis;

    CGI *findCgiByFd(int fd);
    void cleanupCgiForClient(const Client &client);
    void handleCgiIO(int fd, short revents);
    void buildAndSendCgiResponse(Client &cl, const std::string &out);

    void initListeners();
    void createListeningSockets();
    void closeAllSockets();

    void init_pollfds(std::vector<struct pollfd> &pfds);
    void handleClientIO(int fd, short revents);
    void acceptNewClient(int listenFd);
    void closeClientConnexion(int fd);
    void parseRequestLineAndHeaders(Client &client,
                                    const std::string &headersPart);
    void ReadfromClient(Client &client);
    const Block *chooseServerBlock(Client &client);
    const Block *findLocationBlock(const Block &serverBlock,
                                   const std::string &path);
    void readChunkedBody(Client &client);
    bool isMethodAllowed(const Client &client) const;
    std::string buildFullPath(const std::string &requestPath,
                              const std::string &root,
                              const std::string &locationPrefix) const;
    void generateErrorResponse(Client &client, int status);
    void handleGET(Client &client, const std::string &path);
    void handlePOST(Client &client, const std::string &path);
    void handleDELETE(Client &client, const std::string &path);
    void handleRequest(Client &client);
    void WritetoClient(Client &client);
};

size_t getMaxBodySizeFromConfig(const Block *loc, const Block *srv);
std::string trim(const std::string &s);
std::string toLower(std::string s);
std::string sizeToStr(size_t n);
std::string getPathOnly(const std::string &target);

const key *findKey(const Block &block, const std::string &name); // TODO:
std::vector<std::string> getStringValues(const Block &block,
                                         const std::string &name);
std::vector<int> getIntValues(const Block &block, const std::string &name);
std::string getRoot(const Client &client);

#endif
