
#include "../../inc/Server/TCPserver.hpp"

std::string getExtension(const std::string &path)
{
	size_t dot = path.rfind('.');
	if (dot == std::string::npos)
		return "";
	return path.substr(dot + 1);
}

std::string getContentType(const std::string &path)
{
	std::string ext = getExtension(path);

	if (ext == "html" || ext == "htm")
		return "text/html";
	if (ext == "css")
		return "text/css";
	if (ext == "js")
		return "application/javascript";
	if (ext == "json")
		return "application/json";
	if (ext == "txt")
		return "text/plain";
	if (ext == "png")
		return "image/png";
	if (ext == "jpg" || ext == "jpeg")
		return "image/jpeg";
	if (ext == "pdf")
		return "application/pdf";

	return "application/octet-stream";
}

void TCPserver::handleGET(Client &client, const std::string &path)
{
	std::ifstream file(path.c_str(), std::ios::in | std::ios::binary);
	if (!file) {
		generateErrorResponse(client, 404);
		return;
	}

	std::ostringstream oss;
	oss << file.rdbuf();
	std::string body = oss.str();

	std::string header = "HTTP/1.1 200 OK\r\n"
							"Content-Length: " +
							intToStr((int)body.size()) +
							"\r\n"
							"Content-Type: " +
							getContentType(path) +
							"\r\n"
							"Connection: close\r\n\r\n";

	client.sendBuffer = header + body;
	client.state = SEND_RESPONSE;
}
