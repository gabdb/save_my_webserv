/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ConfigChecks.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nicolive <nicolive@student.42belgium.be    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/13 11:06:24 by nicolive          #+#    #+#             */
/*   Updated: 2026/01/13 14:34:18 by nicolive         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../inc/config/Config.hpp"

bool Config::isConfigFileValid(const std::string filename) {
	std::string extension = ".conf";
	if (filename.length() >= extension.length()) {
		if (filename.compare(filename.length() - extension.length(),
							 extension.length(), extension) == 0)
			return true;
	}
	return false;
}

bool Config::isValidDirectiveName(const std::string &s) {
	static std::set<std::string> allowed;
	allowed.insert("listen");
	allowed.insert("host");
	allowed.insert("server_name");
	allowed.insert("root");
	allowed.insert("index");
	allowed.insert("methods");
	allowed.insert("allowed_size");
	allowed.insert("error_page");
	allowed.insert("limit_body_size");
	allowed.insert("client_max_body_size");
	allowed.insert("allowed_methods");
	allowed.insert("autoindex");
	allowed.insert("index");
	allowed.insert("include");
	allowed.insert("cgi_param");
	allowed.insert("include");
	allowed.insert("return");

	return allowed.count(s) > 0;
}

void Config::checkSemicolons(const std::string &content) {
	std::istringstream is(content);
	std::string line;

	int lineNumber = 0;

	while (getline(is, line)) {
		lineNumber++;

		size_t hashPos = line.find('#');
		if (hashPos != std::string::npos)
			line = line.substr(0, hashPos);

		size_t start = line.find_first_not_of(" \t\n\r");
		if (start == std::string::npos) {
			line.clear();
			continue;
			;
		}

		size_t end = line.find_last_not_of(" \t\n\r");
		line.erase(end + 1);
		line.erase(0, start);

		if (line[line.size() - 1] == '{' || line[line.size() - 1] == '}')
			continue;

		if (line[line.size() - 1] != ';') {
			throw std::runtime_error("Missing semicolon ';' at line " +
									 intToStr(lineNumber) + ": \"" + line +
									 "\"");
		}
	}
}

void Config::checkBrackets(const std::string &content) {
	int count = 0;

	for (size_t i = 0; i < content.size(); ++i) {
		char c = content[i];
		if (c == '#') {
			while (i < content.size() && content[i] != '\n')
				++i;
			continue;
		}
		if (c == '{')
			count++;
		else if (c == '}')
			count--;
		if (count < 0)
			throw std::runtime_error("Unexpected '}' (closing bracket without "
									 "matching opening bracket)");
	}
	if (count > 0)
		throw std::runtime_error("Missing '}' closing brackets");
}

void Config::checkDuplicatePort(const std::vector<int> &listeningPorts) {
	std::string errors;
	int port1;
	int port2;
	for (size_t i = 0; i < listeningPorts.size() - 1; i++) {
		port1 = listeningPorts[i];
		for (size_t j = i + 1; j < listeningPorts.size(); j++) {
			port2 = listeningPorts[j];
			// std::cout << i << port1 <<" , " << j <<  port2 <<std::endl;
			if (port1 == port2)
				throw std::runtime_error(
					"Duplicate listen directive inside the same server: " +
					intToStr(port1) + " : " + intToStr(port2));
		}
	}
}

void Config::checkClientBodySizeValue(const std::vector<std::string> &tokens, const size_t pos) {
	for (size_t k = 0; k < tokens[pos].size(); ++k) {
				char c = tokens[pos][k];
				if (!isdigit(c))
					throw std::runtime_error("Invalid client_max_body_size format: " +
											 tokens[pos]);
			}
			int p = 0;
			try {
				p = strToInt(tokens[pos]);
			} catch (...) {
				throw std::runtime_error("Invalid client_max_body_size: '" + tokens[pos] + "'");
			}
			if (p < 1 || p > 1073741824)
				throw std::runtime_error("client_max_body_size out of range: " + tokens[pos]);
}

void Config::checkPortValue(const std::vector<std::string> &tokens, const size_t pos, Block &block) {
	for (size_t k = 0; k < tokens[pos].size(); ++k) {
				char c = tokens[pos][k];
				if (!isdigit(c))
					throw std::runtime_error("Invalid port format: " +
											 tokens[pos]);
			}
			int p = 0;
			try {
				p = strToInt(tokens[pos]);
			} catch (...) {
				throw std::runtime_error("Invalid port: '" + tokens[pos] + "'");
			}
			if (p < 1 || p > 65535)
				throw std::runtime_error("Port out of range: " + tokens[pos]);

			block.listeningPorts.push_back(p);
}

void Config::checkAllowedMethod(const std::vector<std::string> &tokens, const size_t pos) {
	if (tokens[pos] != "GET" && tokens[pos] != "POST" && tokens[pos] != "DELETE")
		throw std::runtime_error("Invalid method: '" + tokens[pos] + "', only GET, POST or DELETE");
}
