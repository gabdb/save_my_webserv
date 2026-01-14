/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   getterConfig.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nicolive <nicolive@student.42belgium.be    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/13 13:04:06 by nicolive          #+#    #+#             */
/*   Updated: 2026/01/09 13:39:00 by nicolive         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../inc/config/Config.hpp"

const std::vector<Block> &Config::getServer() const { return this->_servers; }
