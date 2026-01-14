/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Config.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nicolive <nicolive@student.42belgium.be    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/12 17:44:08 by nicolive          #+#    #+#             */
/*   Updated: 2026/01/13 13:59:58 by nicolive         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef __CONFIG_HPP__
#define __CONFIG_HPP__

#include <fstream>
#include <iostream>
#include <set>
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

struct Block {
    std::string name;
    std::vector<std::string> paths;
    std::vector<key> keys;
    std::vector<Block> locations;
    std::vector<int> listeningPorts;
};

class Config {
  public:
    Config(const std::string filename);
    ~Config();

    std::vector<int> getListeningPorts() const;
    const std::vector<Block> &getServer() const;
    void debugPrintConfig() const;

  private:
    std::vector<Block> _servers;
    std::vector<std::string> _hosts;

    void parseConfig(const std::string filename);

    bool isConfigFileValid(const std::string filename);
    bool isValidDirectiveName(const std::string &s);
    void checkDuplicatePort(const std::vector<int> &listeningPorts);
    void checkBrackets(const std::string &content);
    void checkSemicolons(const std::string &content);
	void checkClientBodySizeValue(const std::vector<std::string> &tokens, const size_t pos);
	void checkPortValue(const std::vector<std::string> &tokens, const size_t pos, Block &block);
	void checkAllowedMethod(const std::vector<std::string> &tokens, const size_t pos);

	
    std::vector<std::string> tokenization(const std::string &content);
    Block parseBlock(std::vector<std::string> &tokens, size_t &pos);
    key parseKeys(std::vector<std::string> &tokens, size_t &pos, Block &block);
};

#endif
