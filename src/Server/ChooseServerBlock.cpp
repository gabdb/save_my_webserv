
#include "../../inc/Server/TCPserver.hpp"
#include "../../inc/temp.hpp" // header miteux ?

//correspond au 'site' demandé par la requete HTTP (Host: xxx) avec server_name dans config
const Block* TCPserver::chooseServerBlock(Client &client)
{
	// retrouver le listener correspondant au client
	const Listener *lst = NULL;

	for (size_t i = 0; i < _listeners.size(); i++)
	{
		if (_listeners[i].fd == client.listenerFd)
		{
			lst = &_listeners[i];
			break;
		}
	}

	// Sécurité : si aucun listener trouvé (ne devrait pas arriver)
	if (!lst || lst->servers.empty())
		return NULL; // p-e mieux de throw error ?

	// si un seul server block -> pas besoin de chercher
	if (lst->servers.size() == 1)
		return lst->servers[0];

	// sinon -> chercher selon Host:
	std::map<std::string, std::string>::iterator it;
	it = client.request.headers.find("Host"); // l'iterator pointe vers la key/value pair 
	if (it != client.request.headers.end())
	{
		std::string hostValue = it->second; // first == 'Host' et second == par ex: 'localhost:8080'
		
		// p-e qu'il faudra enlever la partie ':8080' de la second value dictionnaire 
		
		const Block *srvBlock;
		const key *k;
		for (size_t i = 0; i < lst->servers.size(); i++)
		{
			srvBlock = lst->servers[i];
			k = findKey(*srvBlock, "server_name");
			if (k && !k->values.empty())
			{
				if (k->values[0] == hostValue)
					return srvBlock;
			}
		}
	}
	// "Host" pas trouvé -> "fallback : prendre le premier comme dans NGINX
	return lst->servers[0];
}
