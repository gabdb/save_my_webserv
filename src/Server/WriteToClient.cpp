
#include "../../inc/Server/TCPserver.hpp"

void TCPserver::WritetoClient(Client &client)
{
	// rien à renvoyer
	if (client.sendBuffer.empty())
	{
		client.wantWrite = false;
		client.state = CLOSE_CONNECTION;
		return;
	}

	ssize_t bytes = send(client.fd, client.sendBuffer.c_str(), client.sendBuffer.size(), 0);

	client.lastActivity = time(NULL);

	// - bytes < 0  : send() peut échouer temporairement (EAGAIN).
	//               Sujet: pas d'utilisation de errno après write/send → on NE ferme PAS.
	if (bytes == 0) //client a coupé connection
	{
		client.state = CLOSE_CONNECTION;
		return;
	}
	if (bytes < 0)
		return;

	client.sendBuffer.erase(0, bytes);

	if (client.sendBuffer.empty())
	{
		client.wantWrite = false;
		client.state = CLOSE_CONNECTION;
	}
}
