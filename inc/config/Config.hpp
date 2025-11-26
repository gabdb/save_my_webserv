/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Config.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nicolive <nicolive@student.s19.be>         +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/12 17:44:08 by nicolive          #+#    #+#             */
/*   Updated: 2025/11/13 14:29:10 by nicolive         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef __CONFIG_HPP__
#define __CONFIG_HPP__

#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "../../inc/utils/utils.hpp"

#define DEFAULT_CONFIG_PATH "./config/config.conf"

struct key {
  std::string name;
  std::vector<std::string> values;
};

// Nico TODO: si dans "location" block, s'assurer que <Block> location n'est pas accessible (non authorisé), location { location{...} } = faux
// Nico TODO: populate _ListeningPorts and _hosts via parseKeys or adjust logic to simply save as lambda key:value and let me fetch them later
struct Block {
  std::string name;               // "server" or "location"
  std::vector<std::string> paths; // e.g. "/" for location / { ... }
  std::vector<key> keys;          // directives like listen, host, root...
  std::vector<Block> location;    // changed to location for easier readability (no other possibility anyways)
  // To populate
  std::vector<int> _listeningPorts;
  std::vector<std::string> _hosts;
};

class Config {
public:
  Config(const std::string filename);
  ~Config();

  std::vector<int> getListeningPorts() const;
  std::vector<std::string> getHost() const;
  const std::vector<Block> &getServer() const; // changed to '&' so Listener struct can point to respective blocks.
  void debugPrintConfig() const;

private:
  std::vector<Block> _servers;
  std::vector<int> _listeningPorts;
  std::vector<std::string> _hosts;

  void parseConfig(const std::string filename);

  bool isConfigFileValid(const std::string filename);
  void checkDuplicatePort(const std::vector<int> &_listeningPorts);

  std::vector<std::string> tokenization(const std::string &content);
  Block parseBlock(std::vector<std::string> &tokens, size_t &pos); // modified logic - block.child.pushback of a server token is not acceptable in config file (nested server should not work) + modified .child to .location for readability
  key parseKeys(std::vector<std::string> &tokens, size_t &pos);
};

#endif
