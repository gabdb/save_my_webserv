/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gnyssens <gnyssens@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/12 16:56:24 by nicolive          #+#    #+#             */
/*   Updated: 2026/01/05 13:57:58 by gnyssens         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <iostream>
#include <stdexcept>
#include "../inc/config/Config.hpp"
#include "../inc/Server/TCPserver.hpp"

int main(int ac, char **av)
{
    if (ac != 2)
    {
        std::cerr << "Usage: ./webserv <config_file>\n";
        return 1;
    }

    try
    {
        Config cfg(av[1]);       // parse + checks
        TCPserver srv(cfg);      // build listeners/servers
        srv.init();              // socket/bind/listen
        std::cout << "webserv: running with config: " << av[1] << std::endl;
        srv.run();               // poll loop (never returns)
    }
    catch (const std::exception &e)
    {
        std::cerr << "webserv: fatal error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}


/*
#include "../inc/temp.hpp"

int main(int argc, char const *argv[]) {
  std::string config_path;

  if (argc == 1)
    config_path = DEFAULT_CONFIG_PATH;
  else if (argc == 2)
    config_path = argv[1];
  else {
    std::cerr << "Error: too many arguments" << std::endl;
    return 1;
  }

  try {
    // Nico TODO: Bug in parseKeys()
    Config config(config_path);
    config.debugPrintConfig();
    TCPserver server(config);

    server.init();
    const std::vector<Listener> &listeners = server.getListeners();
    for (size_t i = 0; i < listeners.size(); ++i) {
      std::cout << "Listening on host='" << listeners[i].host
                << "' port=" << listeners[i].port
                << " fd=" << listeners[i].fd << std::endl;
    }
    // Gab TODO: Focus on .run(), add up to TCPserver or create your own class (TCPserver prefered)
    server.run();
  } catch (const std::exception &e) {
    std::cerr << "Fatal error: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}
*/
