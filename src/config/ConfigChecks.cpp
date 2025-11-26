/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ConfigChecks.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nicolive <nicolive@student.s19.be>         +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/13 11:06:24 by nicolive          #+#    #+#             */
/*   Updated: 2025/11/13 11:45:25 by nicolive         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../inc/config/Config.hpp"

bool Config::isConfigFileValid(const std::string filename)
{
	std::string extension = ".conf";
	if (filename.length() >= extension.length())
	{
		if (filename.compare(filename.length() - extension.length(), extension.length(), extension) == 0 )
			return true;
	}
	return false;
}

void Config::checkDuplicatePort(const std::vector<int> &_listeningPorts)
{
	std::string errors;
	int port1;
	int port2;
	for (size_t i = 0; i < _listeningPorts.size() - 1; i++)
	{
		port1 = _listeningPorts[i];
		for (size_t j = i + 1; j < _listeningPorts.size(); j++)
		{
			port2 = _listeningPorts[j];
			if (port1 == port2)
				errors += intToStr(port1) + " ";
		}
	}
	
}
