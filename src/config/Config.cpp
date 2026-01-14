/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Config.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nicolive <nicolive@student.42belgium.be    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/12 17:44:17 by nicolive          #+#    #+#             */
/*   Updated: 2026/01/13 14:02:55 by nicolive         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../inc/config/Config.hpp"

Config::Config(const std::string filename) {
	if (!isConfigFileValid(filename))
		throw std::runtime_error(
			"Type file for config not valid, expecting .conf file");
	parseConfig(filename);
}

Config::~Config() {}

void Config::parseConfig(const std::string filename) {
	std::ifstream file(filename.c_str());
	if (!file.is_open())
		throw std::runtime_error("Cannot open config file : " + filename);

	std::stringstream buffer;
	buffer << file.rdbuf();
	std::string content = buffer.str();
	file.close();

	checkBrackets(content);
	checkSemicolons(content);

	std::vector<std::string> tokens = tokenization(content);

	//------------FOR DEBUG---------------
	// for (size_t i = 0; i < tokens.size(); i++) {
	//     std::cout << tokens[i] << std::endl;
	// }
	size_t pos = 0;

	while (pos < tokens.size()) {
		if (tokens[pos] == "server") {
			_servers.push_back(parseBlock(tokens, pos));
		} else {
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
			while (i < content.size() &&
				   !std::isspace(static_cast<unsigned char>(content[i])) &&
				   content[i] != '{' && content[i] != '}' &&
				   content[i] != ';' && content[i] != '#') {
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
		if (tokens[pos] == "location" && block.name == "server")
			block.locations.push_back(parseBlock(tokens, pos));
		else if (tokens[pos] == "location" && block.name != "server")
			throw std::runtime_error(
				"Location {} need to be inside a server{} and not " +
				block.name + " {}");
		else
			block.keys.push_back(parseKeys(tokens, pos, block));
	}
	if (block.name == "server")
		checkDuplicatePort(block.listeningPorts);
	if (pos == tokens.size() || tokens[pos] != "}")
		throw std::runtime_error("Expected '}' to close block " + block.name);

	++pos;
	return block;
}

key Config::parseKeys(std::vector<std::string> &tokens, size_t &pos, Block &block) {
	key keys;

	keys.name = tokens[pos++];

	if (!isValidDirectiveName(keys.name))
		throw std::runtime_error("Unknown directive: " + keys.name);

	while (pos < tokens.size() && tokens[pos] != ";") {
		if (keys.name == "client_max_body_size") {
			checkClientBodySizeValue(tokens, pos);
		}
		else if (keys.name == "listen") {
			checkPortValue(tokens, pos, block);
		} else if (keys.name == "allowed_methods") {
			checkAllowedMethod(tokens, pos);
		} else if (keys.name == "host")
			_hosts.push_back(tokens[pos]);

		keys.values.push_back(tokens[pos++]);
	}

	// check if keys is empty for host set to default 0.0.0.0 else error
	if (keys.values.empty()) {
		if (keys.name == "host") {
			_hosts.push_back("0.0.0.0");
			keys.values.push_back("0.0.0.0");
		} else
			throw std::runtime_error("Directive '" + keys.name +
									 "' must have at least one value");
	}

	for (size_t i = 0; i < keys.values.size(); ++i) {
		std::string &v = keys.values[i];
		if (v.find('{') != std::string::npos ||
			v.find('}') != std::string::npos ||
			v.find(';') != std::string::npos) {
			throw std::runtime_error("Invalid value inside Directive '" +
									 keys.name + "' : '" + v + "'");
		}
	}

	if (pos == tokens.size() || tokens[pos] != ";")
		throw std::runtime_error("Expected ';' after directive: " + keys.name);

	++pos;
	return keys;
}

