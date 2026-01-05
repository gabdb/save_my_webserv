/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   TCPserver.hpp                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gnyssens <gnyssens@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/13 13:56:07 by nicolive          #+#    #+#             */
/*   Updated: 2026/01/04 18:05:19 by gnyssens         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef __TCPSERVER_HPP__
#define __TCPSERVER_HPP__

#include <cstdlib>
#include <fcntl.h>
#include <map>
#include <netinet/in.h>
#include <poll.h>
#include <sstream>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <fstream>
#include <sys/stat.h>
#include <dirent.h>
#include <algorithm>
#include <ctime>

#include "../config/Config.hpp"

class CGI;

// Server host:port entries
struct Listener {
  int fd;
  std::string host;
  int port;
  std::vector<const Block *> servers; // pointers to respective server blocks who are using this listener
};

struct HttpRequest {
  std::string method;  // GET/POST/DELETE
  std::string target;  // URL demandée par le client
  std::string version; // souvent: HTTP/1.1

  std::map<std::string, std::string> headers; // dictionnaire de headers (par ex. `Host: localhost:8080`)
  std::string body;                           // surtout utile pr POST

  bool headersComplete; // devient true quand "\r\n\r\n"
  bool bodyComplete;    // devient true si bodyReceived == bodyExpected
};

enum ClientState {
  READ_REQUEST,
  PARSE_REQUEST,
  HANDLE_REQUEST,
  WAIT_CGI,
  SEND_RESPONSE,
  CLOSE_CONNECTION
};

struct Client {
  int fd;
  std::string recvBuffer;
  std::string sendBuffer;
  bool wantRead;
  bool wantWrite;
  const Block *serverBlock;
  bool keepAlive; //p-e retirer

  // rajout gab
  const Block *locationBlock; // determined after parsing the request path (== sous-block en fonction du path/url dans la requete http)
  HttpRequest request;        // parsed request data
  ClientState state;   // enum
  size_t bodyExpected; // Content-Length
  size_t bodyReceived; // to track when body is complete
  int listenerFd; // fd du listener qui a accepté ce client^

  time_t lastActivity;
  
  // HttpResponse response; ???
};

class TCPserver {
public:
  TCPserver(const Config &cfg);
  ~TCPserver();

  const std::vector<Listener> &getListeners() const;

  void init();
  void run();

  void startCgiForClient(Client &client,
                         const std::string &interpreterPath,
                         const std::string &scriptPath,
                         const std::string &queryString);
  bool isCgiFinishedForClient(const Client &client) const;
  std::string getCgiOutputForClient(const Client &client);

private:
  const Config &_config;
  std::vector<Listener> _listeners;
  std::map<int, Client> _clients; // fd, client
  std::map<int, CGI *> _cgis;     // key: CGI stdout fd

  CGI *findCgiByFd(int fd);
  const CGI *findCgiByFd(int fd) const;
  void cleanupCgiForClient(const Client &client);
  void handleCgiIo(int fd, short revents);

  // Listener (Server) stuff -> init ()
  void initListeners();          // maps out port:host listeners for server
  void createListeningSockets(); // inits sockets for each port:host
  void closeAllSockets();

  // Client (browser) <--TCP connexion--> Server stuff -> run()
  void init_pollfds(std::vector<struct pollfd> &pfds);
  void acceptNewClient(int listenFd);
  void parseRequestLineAndHeaders(Client &client, const std::string &headersPart);
  void ReadfromClient(Client &client);
  const Block* chooseServerBlock(Client &client);
  const Block* findLocationBlock(const Block &serverBlock, const std::string &path);
  bool isMethodAllowed(const Client &client) const;
  std::string getRoot(const Client &client) const;
  std::string buildFullPath(const std::string &requestPath, const std::string &root, const std::string &locationPrefix) const;
  void generateErrorResponse(Client &client, int status);
  void handleGET(Client &client, const std::string &path);
  void handlePOST(Client &client, const std::string &path);
  void handleDELETE(Client &client, const std::string &path);
  void handleRequest(Client &client);
  void WritetoClient(Client &client);
  void closeClientConnexion(int fd);
};

void buildAndSendCgiResponse(Client &cl, const std::string &out);

// Helpers to navigate key/value pairs
const key *findKey(const Block &block, const std::string &name);                   // TODO:
std::vector<const key *> findAllKeys(const Block &block, const std::string &name); // TODO:
std::vector<std::string> getStringValues(const Block &block, const std::string &name);
std::vector<int> getIntValues(const Block &block, const std::string &name);


#endif
