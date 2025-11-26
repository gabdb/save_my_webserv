/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   TCPserver.cpp                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gmiorcec <gmiorcec@student.s19.be>         +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/22 09:48:26 by gmiorcec          #+#    #+#             */
/*   Updated: 2025/11/23 21:26:00 by gmiorcec         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../inc/Server/TCPserver.hpp"
#include <arpa/inet.h>
#include <cstring>
#include <stdexcept>
#include <unistd.h>

TCPserver::TCPserver(const Config &cfg)
    : _config(cfg) {
}

TCPserver::~TCPserver() {
  closeAllSockets();
}

void TCPserver::init() {
  try {
    initListeners();
    createListeningSockets();
  } catch (...) {
    throw;
  }
}

// const std::vector<int> &TCPserver::getListenFds() const {
//   return _listenFds;
// }

const std::vector<Listener> &TCPserver::getListeners() const {
  return _listeners;
}

std::vector<std::string> getStringValues(const Block &block, const std::string &name) {
  std::vector<std::string> result;
  for (size_t i = 0; i < block.keys.size(); ++i) {
    if (block.keys[i].name == name && !block.keys[i].values.empty()) {
      for (size_t j = 0; j < block.keys[i].values.size(); ++j)
        result.push_back(block.keys[i].values[j]);
    }
  }
  if (result.empty()) {
    throw std::runtime_error("string value for key " + name + " not found");
  }
  return result;
}

std::vector<int> getIntValues(const Block &block, const std::string &name) {
  std::vector<int> result;
  for (size_t i = 0; i < block.keys.size(); ++i) {
    if (block.keys[i].name == name) {
      for (size_t v = 0; v < block.keys[i].values.size(); ++v) {
        result.push_back(strToInt(block.keys[i].values[v]));
      }
    }
  }
  if (result.empty())
    throw std::runtime_error("int value for key " + name + " not found");
  return result;
}

void TCPserver::initListeners() {

  const std::vector<Block> &servers = _config.getServer();
  std::map<std::string, Listener> listenerMap;

  for (size_t i = 0; i < servers.size(); ++i) {
    const Block &srvBlock = servers[i];

    if (srvBlock.name != "server") {
      throw std::runtime_error("Top-level block is not 'server'");
    }

    std::vector<std::string> hosts = getStringValues(srvBlock, "host");
    // TODO: enhance error handling logic within getStringValues

    std::vector<int> ports = getIntValues(srvBlock, "listen");

    // Check if "host:port" is allready in map. If yes, append to key's server list. Otherways, create mapping
    for (size_t h = 0; h < hosts.size(); ++h) {
      for (size_t p = 0; p < ports.size(); ++p) {
        const int port = ports[p];

        std::string key = hosts[h] + ":" + intToStr(port);

        std::map<std::string, Listener>::iterator it = listenerMap.find(key);
        if (it == listenerMap.end()) {
          Listener lst;
          lst.fd = -1;
          lst.host = hosts[h];
          lst.port = port;
          lst.servers.push_back(&srvBlock);

          listenerMap.insert(std::make_pair(key, lst));
        }
        else {
          it->second.servers.push_back(&srvBlock);
        }
      }
    }
  }
  for (std::map<std::string, Listener>::iterator it = listenerMap.begin(); it != listenerMap.end(); ++it)
    _listeners.push_back(it->second);
}

void TCPserver::closeAllSockets() {
  for (size_t i = 0; i < _listeners.size(); ++i) {
    if (_listeners[i].fd >= 0) {
      ::close(_listeners[i].fd);
      _listeners[i].fd = -1;
    }
  }
}

void TCPserver::createListeningSockets() {

  for (size_t i = 0; i < _listeners.size(); ++i) {

    Listener &lst = _listeners[i];
    int opt = 1;
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(lst.port);

    if (fd < 0)
      throw std::runtime_error("socket() failed for listener");
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
      throw std::runtime_error("setsockopt(SO_REUSEADDR) failed");
    if (lst.host.empty())
      addr.sin_addr.s_addr = htonl(INADDR_ANY);
    else {
      in_addr_t ip = inet_addr(lst.host.c_str());
      if (ip == INADDR_NONE)
        throw std::runtime_error("Invalid IP address in host: " + lst.host);
      addr.sin_addr.s_addr = ip;
    }
    if (bind(fd, reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr)) < 0)
      throw std::runtime_error("bind() failed on " + lst.host + ":" + intToStr(lst.port));
    if (listen(fd, SOMAXCONN) < 0)
      throw std::runtime_error("listen() failed on " + lst.host + ":" + intToStr(lst.port));
    if (fcntl(fd, F_SETFL, O_NONBLOCK) < 0)
      throw std::runtime_error("fcntl(O_NONBLOCK) failed on listening socket");

    lst.fd = fd;
  }
}
