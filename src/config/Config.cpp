/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Config.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nicolive <nicolive@student.s19.be>         +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/12 17:44:17 by nicolive          #+#    #+#             */
/*   Updated: 2025/11/13 18:35:43 by nicolive         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../inc/config/Config.hpp"

Config::Config(const std::string filename) {
  if (!isConfigFileValid(filename))
    throw std::runtime_error("Type file for config not valid, expecting .conf file");
  parseConfig(filename);
}

Config::~Config() {
}

void Config::parseConfig(const std::string filename) {
  std::ifstream file(filename.c_str());
  if (!file.is_open())
    throw std::runtime_error("Cannot open config file : " + filename);

  std::stringstream buffer;
  buffer << file.rdbuf();
  std::string content = buffer.str();
  file.close();

  std::vector<std::string> tokens = tokenization(content);
  size_t pos = 0;

  while (pos < tokens.size()) {
    if (tokens[pos] == "server") {
      _servers.push_back(parseBlock(tokens, pos));
    }
    else {
      throw std::runtime_error("Unexpected token: " + tokens[pos]);
    }
  }
  // checkDuplicatePort(_listeningPorts);
}

std::vector<std::string> Config::tokenization(const std::string &content) {
  std::vector<std::string> tokens;
  std::string token;

  for (size_t i = 0; i < content.size(); ++i) {
    char c = content[i];
    if (c == '#') {
      while (i < content.size() && content[i] != '\n')
        ++i;
      continue;
    }
    if (std::isspace(static_cast<unsigned char>(c)))
      continue;
    else if (c == '{' || c == '}' || c == ';')
      tokens.push_back(std::string(1, c));
    else {
      token.clear();
      while (i < content.size() && !std::isspace(static_cast<unsigned char>(content[i])) &&
             content[i] != '{' && content[i] != '}' && content[i] != ';' && content[i] != '#') {
        token += content[i];
        ++i;
      }
      if (i < content.size() && content[i] == '#') {
        while (i < content.size() && content[i] != '\n')
          ++i;
      }
      --i;
      if (!token.empty())
        tokens.push_back(token);
    }
  }

  return tokens;
}

Block Config::parseBlock(std::vector<std::string> &tokens, size_t &pos) {
  Block block;
  block.name = tokens[pos++];

  while (pos < tokens.size() && tokens[pos] != "{") {
    block.paths.push_back(tokens[pos++]);
  }

  if (pos == tokens.size() || tokens[pos] != "{")
    throw std::runtime_error("Expected '{' after block name " + block.name);

  ++pos;
  while (pos < tokens.size() && tokens[pos] != "}") {
    if (tokens[pos] == "location") // removed '|| tokens[pos] == "server"'
      block.location.push_back(parseBlock(tokens, pos));
    else {
      block.keys.push_back(parseKeys(tokens, pos));
    }
  }

  if (pos == tokens.size() || tokens[pos] != "}")
    throw std::runtime_error("Expected '}' to close block " + block.name);

  ++pos;
  return block;
}

key Config::parseKeys(std::vector<std::string> &tokens, size_t &pos) {
  key keys;

  keys.name = tokens[pos++];

  while (pos < tokens.size() && tokens[pos] != ";") {
    // const std::string &value = tokens[pos];
    if (keys.name == "listen")
      // I messed up this funciton, sorry '-_-
      // Nico TODO: Check if we have multiple ports to listen to (listen may contain several ports)
      // Nico TODO: Can you make _hosts and _listeningPorts stored within the "Block" struct instead of "Config" (easier to access and setup listener that way)
      // Nico TODO: fix logic to store key:value(s) correctly
      _listeningPorts.push_back(strToInt(tokens[pos]));
    else if (keys.name == "host")
      _hosts.push_back(tokens[pos]);
    keys.values.push_back(tokens[pos++]);
  }

  if (pos == tokens.size() || tokens[pos] != ";")
    throw std::runtime_error("Expected ';' after directive: " + keys.name);

  ++pos;
  return keys;
}

static std::string indentString(int indent) {
  std::string s;
  for (int i = 0; i < indent; i++)
    s += "\t";
  return s;
}

static void printBlock(const Block &block, int indent = 0) {
  std::string tab = indentString(indent);

  std::cout << tab << "Block: " << block.name << std::endl;

  if (!block.paths.empty()) {
    std::cout << tab << "Paths: ";
    for (size_t i = 0; i < block.paths.size(); i++) {
      std::cout << block.paths[i] << " ";
    }
    std::cout << std::endl;
  }

  for (size_t i = 0; i < block.keys.size(); i++) {
    const key &k = block.keys[i];

    std::cout << tab << "Key: " << k.name << " = ";

    for (size_t j = 0; j < k.values.size(); j++) {
      std::cout << k.values[j] << " ";
    }
    std::cout << std::endl;
  }

  for (size_t i = 0; i < block.location.size(); i++) {
    printBlock(block.location[i], indent + 1);
  }
}

void Config::debugPrintConfig() const {
  for (size_t i = 0; i < _servers.size(); i++) {
    printBlock(_servers[i], 0);
  }
}
