/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nicolive <nicolive@student.42belgium.be    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/12 16:56:24 by nicolive          #+#    #+#             */
/*   Updated: 2026/01/09 13:03:45 by nicolive         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../inc/Server/TCPserver.hpp"
#include "../inc/config/Config.hpp"
#include <iostream>
#include <stdexcept>

int main(int ac, char **av) {
    if (ac != 2) {
        std::cerr << "Usage: ./webserv <config_file>\n";
        return 1;
    }

    try {
        Config cfg(av[1]); // parse + checks
        cfg.debugPrintConfig();
        TCPserver srv(cfg); // build listeners/servers
        srv.init();         // socket/bind/listen
        std::cout << "webserv: running with config: " << av[1] << std::endl;
        srv.run(); // poll loop (never returns)
    } catch (const std::exception &e) {
        std::cerr << "webserv: fatal error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
