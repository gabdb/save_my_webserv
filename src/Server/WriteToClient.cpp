
#include "../../inc/Server/TCPserver.hpp"

void TCPserver::WritetoClient(Client &client)
{
	if (client.sendBuffer.empty()) {
		std::cout << "Empty Buffer" << std::endl;
		client.state = CLOSE_CONNECTION;
		return;
	}

	ssize_t bytes = send(client.fd, client.sendBuffer.c_str(), client.sendBuffer.size(), 0);
	client.lastActivity = time(NULL);

	if (bytes <= 0) {
		client.state = CLOSE_CONNECTION;
		return;
	}

	client.sendBuffer.erase(0, static_cast<size_t>(bytes));

	if (client.sendBuffer.empty()) {
		client.state = CLOSE_CONNECTION;
	}
}
