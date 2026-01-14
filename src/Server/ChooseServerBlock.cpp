
#include "../../inc/Server/TCPserver.hpp"
#include "../../inc/temp.hpp"

const Block *TCPserver::chooseServerBlock(Client &client)
{
	const Listener *lst = NULL;

	for (size_t i = 0; i < _listeners.size(); i++) {
		if (_listeners[i].fd == client.listenerFd) {
			lst = &_listeners[i];
			break;
		}
	}

	if (!lst || lst->servers.empty())
		return NULL;

	if (lst->servers.size() == 1)
		return lst->servers[0];

	std::map<std::string, std::string>::iterator it;
	it = client.request.headers.find(
		"Host");
	if (it != client.request.headers.end()) {
		std::string hostValue =
			it->second;

		size_t check = hostValue.find(':');
		if (check != std::string::npos) {
			hostValue = hostValue.substr(0, check);
		}

		const Block *srvBlock;
		const key *k;
		for (size_t i = 0; i < lst->servers.size(); i++) {
			srvBlock = lst->servers[i];
			k = findKey(*srvBlock, "server_name");
			if (k && !k->values.empty()) {
				if (k->values[0] == hostValue)
					return srvBlock;
			}
		}
	}
	return lst->servers[0];
}
