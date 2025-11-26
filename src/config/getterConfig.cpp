/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   getterConfig.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nicolive <nicolive@student.s19.be>         +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/13 13:04:06 by nicolive          #+#    #+#             */
/*   Updated: 2025/11/13 14:08:02 by nicolive         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../inc/config/Config.hpp"

std::vector<int> Config::getListeningPorts() const {
  return this->_listeningPorts;
}

const std::vector<Block> &Config::getServer() const {
  return this->_servers;
}
