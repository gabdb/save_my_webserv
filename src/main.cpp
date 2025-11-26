/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nicolive <nicolive@student.s19.be>         +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/12 16:56:24 by nicolive          #+#    #+#             */
/*   Updated: 2025/11/13 16:07:23 by nicolive         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

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
    // Gab TODO: Focus on .run(), add up to TCPserver or create your own class (TCPserver prefered)

    const std::vector<Listener> &listeners = server.getListeners();
    for (size_t i = 0; i < listeners.size(); ++i) {
      std::cout << "Listening on host='" << listeners[i].host
                << "' port=" << listeners[i].port
                << " fd=" << listeners[i].fd << std::endl;
    }
    server.run();
  } catch (const std::exception &e) {
    std::cerr << "Fatal error: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}
