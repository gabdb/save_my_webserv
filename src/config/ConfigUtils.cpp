/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ConfigUtils.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nicolive <nicolive@student.42belgium.be    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/13 13:36:29 by nicolive          #+#    #+#             */
/*   Updated: 2026/01/13 13:48:59 by nicolive         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../inc/config/Config.hpp"

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
		std::cout << tab << tab << "Paths: ";
		for (size_t i = 0; i < block.paths.size(); i++) {
			std::cout << block.paths[i] << " ";
		}
		std::cout << std::endl;
	}

	for (size_t i = 0; i < block.keys.size(); i++) {
		const key &k = block.keys[i];

		std::cout << tab << tab << "Key: " << k.name << " = ";

		for (size_t j = 0; j < k.values.size(); j++) {
			std::cout << k.values[j] << " ";
		}
		std::cout << std::endl;
	}

	for (size_t i = 0; i < block.locations.size(); i++) {
		printBlock(block.locations[i], indent + 1);
	}
}

void Config::debugPrintConfig() const {
	for (size_t i = 0; i < _servers.size(); i++) {
		printBlock(_servers[i], 0);
	}
}
