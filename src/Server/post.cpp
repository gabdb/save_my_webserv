
#include "../../inc/Server/TCPserver.hpp"
#include "../../inc/utils/utils.hpp"

static void sendHtml(Client &client, int status, const std::string &body)
{
	std::string header = "HTTP/1.1 " + intToStr(status) + " " +
							reasonPhrase(status) +
							"\r\n"
							"Content-Length: " +
							intToStr((int)body.size()) +
							"\r\n"
							"Content-Type: text/html\r\n"
							"Connection: close\r\n\r\n";

	client.sendBuffer = header + body;
	client.state = SEND_RESPONSE;
}

void TCPserver::handlePOST(Client &client, const std::string &path)
{
	size_t slash = path.find_last_of('/');
	if (slash == std::string::npos) {
		generateErrorResponse(client, 400);
		return;
	}

	std::string dir = path.substr(0, slash);
	struct stat st;

	if (dir.empty())
		dir = "/";

	if (stat(dir.c_str(), &st) < 0 || !S_ISDIR(st.st_mode)) {
		generateErrorResponse(client, 404);
		return;
	}

	std::ofstream file(path.c_str(), std::ios::out | std::ios::binary | std::ios::trunc);
	if (!file) {
		generateErrorResponse(client, 403);
		return;
	}

	if (!client.request.body.empty())
		file.write(client.request.body.c_str(), client.request.body.size());

	file.close();

	sendHtml(client, 201, "<html><body><h1>Upload OK</h1></body></html>");
}
