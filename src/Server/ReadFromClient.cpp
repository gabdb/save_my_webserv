
#include "../../inc/Server/TCPserver.hpp"
#include "../../inc/temp.hpp" // header miteux ?

int	ft_atoi(const char *str)
{
	int		sign;
	long	res;
	long	buff;

	sign = 1;
	res = 0;
	buff = 0;
	while (*str == 32 || (*str >= 9 && *str <= 13))
		str++;
	if (*str == '-')
		sign *= (-1);
	if (*str == '-' || *str == '+')
		str++;
	while (*str >= '0' && *str <= '9')
	{
		res = res * 10 + *str++ - '0';
		if (buff > res && sign > 0)
			return (-1);
		else if (buff > res && sign < 0)
			return (0);
		buff = res;
	}
	return ((int) sign * res);
}

void TCPserver::parseRequestLineAndHeaders(Client &client, const std::string &headersPart)
{
	std::istringstream stream(headersPart); // 'faux' fichier stocké en flux (string), permets de lire line by line
	std::string line;

	std::getline(stream, line);  // ex: "GET /index.html HTTP/1.1"
	if (!line.empty() && line[line.size() - 1] == '\r')
		line.erase(line.size() - 1);

	std::istringstream rl(line);
	rl >> client.request.method >> client.request.target >> client.request.version;

	//parse headers
	while (std::getline(stream, line))
	{
		if (!line.empty() && line[line.size() - 1] == '\r')
			line.erase(line.size() - 1);
		if (line.empty()) //fin des headers
			break;

		size_t colon = line.find(':');
		if (colon != std::string::npos)
		{
			std::string key = line.substr(0, colon);
			std::string value = line.substr(colon + 1);
			if (!value.empty() && value[0] == ' ')
				value.erase(0, 1);

			client.request.headers[key] = value; //remplir dictionnaire key:value (dans struct HttpRequest)
			if (key == "Content-Length")
				client.bodyExpected = ft_atoi(value.c_str());
		}
	}
}


void TCPserver::ReadfromClient(Client &client)
{
	char buffer[4096];
	ssize_t bytes = recv(client.fd, buffer, sizeof(buffer), 0);

	// erreurs
	if (bytes <= 0)
	{
		// recv = 0 → client a fermé proprement
		// recv < 0 → erreur (gérée ailleurs par poll)
		client.state = CLOSE_CONNECTION;
		client.wantRead = false;
		return;
	}
	client.recvBuffer.append(buffer, bytes);

	if (!client.request.headersComplete)
	{
		size_t pos = client.recvBuffer.find("\r\n\r\n");

		if (pos != std::string::npos)
		{
			// fin des headers atteint
			client.request.headersComplete = true;
			std::string headersPart = client.recvBuffer.substr(0, pos); // substr pr s'arreter à la fin des headers

			parseRequestLineAndHeaders(client, headersPart);

			client.recvBuffer.erase(0, pos + 4); // retirer la partie headers du buffer "...\r\n\r\n"

			if (client.bodyExpected == 0) //pas de Content-Length, en gros pas un POST
			{
				client.request.bodyComplete = true;
				client.state = HANDLE_REQUEST;
				client.wantRead = false;
				client.wantWrite = false;
				return;
			}
		}
		else
		{
			// pas encore 'trouvé' la fin des headers → attendre plus de bytes
			return;
		}
	}

	if (!client.request.bodyComplete)
	{
		client.request.body.append(client.recvBuffer);
		client.bodyReceived += client.recvBuffer.size();

		client.recvBuffer.clear();

		if (client.bodyReceived >= client.bodyExpected)
		{
			client.request.bodyComplete = true;

			// body complet -> faut traiter requete
			client.state = HANDLE_REQUEST;
			client.wantRead = false;
			return;
		}
	}
}
