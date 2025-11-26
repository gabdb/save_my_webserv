/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   utils_transform.cpp                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nicolive <nicolive@student.s19.be>         +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/13 11:14:16 by nicolive          #+#    #+#             */
/*   Updated: 2025/11/13 13:44:34 by nicolive         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../inc/utils/utils.hpp"

int strToInt(const std::string &str)
{
	std::stringstream ss(str);
	int value;
	ss >> value;
	if (ss.fail())
		throw std::runtime_error("Invalid port value : " + str);
	return value;
}

std::string intToStr(const int &value)
{
	std::stringstream ss;
	ss << value;
	std::string str = ss.str();
	return str;
}
