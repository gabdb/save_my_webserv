
#include "../../inc/Server/TCPserver.hpp"
#include "../../inc/temp.hpp" 

static bool parseSizeTDec(const std::string &s, size_t &out)
{
	if (s.empty())
		return false;
	size_t v = 0;
	for (size_t i = 0; i < s.size(); ++i) {
		if (s[i] < '0' || s[i] > '9')
			return false;
		size_t digit = static_cast<size_t>(s[i] - '0');

		if (v > (static_cast<size_t>(-1) - digit) / 10)
			return false;
		v = v * 10 + digit;
	}
	out = v;
	return true;
}

static bool parseChunkSizeHex(const std::string &line, size_t &out) {
	size_t semi = line.find(';');
	std::string hexPart = trim(line.substr(0, semi));
	if (hexPart.empty())
		return false;

	size_t v = 0;
	for (size_t i = 0; i < hexPart.size(); ++i) {
		char c = hexPart[i];
		size_t d;
		if (c >= '0' && c <= '9')
			d = static_cast<size_t>(c - '0');
		else if (c >= 'a' && c <= 'f')
			d = static_cast<size_t>(10 + (c - 'a'));
		else if (c >= 'A' && c <= 'F')
			d = static_cast<size_t>(10 + (c - 'A'));
		else
			return false;

		if (v > (static_cast<size_t>(-1) - d) / 16)
			return false;
		v = v * 16 + d;
	}

	out = v;
	return true;
}

static const size_t CHUNK_EXPECT_SIZE = static_cast<size_t>(-1);
static const size_t CHUNK_EXPECT_CRLF = static_cast<size_t>(-2);

void TCPserver::parseRequestLineAndHeaders(Client &client, const std::string &headersPart)
{
	std::istringstream stream(headersPart);
	std::string line;

	client.bodyExpected = static_cast<size_t>(-1);
	client.bodyReceived = 0;
	client.chunked = false;
	client.chunkBytesRemaining = 0;

	if (!std::getline(stream, line)) {
		generateErrorResponse(client, 400);
		return;
	}
	if (!line.empty() && line[line.size() - 1] == '\r')
		line.erase(line.size() - 1);

	std::istringstream rl(line);
	if (!(rl >> client.request.method >> client.request.target >> client.request.version))
	{
		generateErrorResponse(client, 400);
		return;
	}

	// headers
	while (std::getline(stream, line)) {
		if (!line.empty() && line[line.size() - 1] == '\r')
			line.erase(line.size() - 1);
		if (line.empty())
			break;

		size_t colon = line.find(':');
		if (colon == std::string::npos)
			continue;

		std::string key = trim(line.substr(0, colon));
		std::string value = trim(line.substr(colon + 1));

		if (key.empty())
			continue;

		std::string lk = toLower(key);
		if (lk == "host") {
			client.request.headers["Host"] = value;
			continue;
		}
		if (lk == "content-type") {
			client.request.headers["Content-Type"] = value;
			continue;
		}

		if (lk == "content-length") {
			size_t v;
			if (!parseSizeTDec(value, v))
			{
				generateErrorResponse(client, 400);
				return;
			}
			client.request.headers["Content-Length"] = value;
			client.bodyExpected = v;
			continue;
		}

		if (lk == "transfer-encoding") {
			client.request.headers["Transfer-Encoding"] = value;
			std::string lv = toLower(value);
			if (lv.find("chunked") != std::string::npos)
			{
				client.chunked = true;
				client.chunkBytesRemaining = CHUNK_EXPECT_SIZE;
				client.bodyExpected = static_cast<size_t>(-1);
			}
			else
			{
				generateErrorResponse(client, 501);
				return;
			}
			continue;
		}

		client.request.headers[key] = value;
	}
}

void TCPserver::readChunkedBody(Client &client)
{
	size_t maxBody = getMaxBodySizeFromConfig(client.locationBlock, client.serverBlock);

	while (true)
	{
		if (client.chunkBytesRemaining == CHUNK_EXPECT_SIZE)
		{
			size_t eol = client.recvBuffer.find("\r\n");
			if (eol == std::string::npos)
				return;

			std::string sizeLine = client.recvBuffer.substr(0, eol);
			client.recvBuffer.erase(0, eol + 2);

			size_t chunkSize;
			if (!parseChunkSizeHex(sizeLine, chunkSize)) {
				generateErrorResponse(client, 400);
				return;
			}

			if (chunkSize == 0)
			{
				if (client.recvBuffer.size() >= 2 &&
					client.recvBuffer[0] == '\r' &&
					client.recvBuffer[1] == '\n') {
					client.recvBuffer.erase(0, 2);
					client.request.bodyComplete = true;
					client.request.headers["Content-Length"] =
						sizeToStr(client.request.body.size());
					return;
				}

				size_t endTrailers = client.recvBuffer.find("\r\n\r\n");
				if (endTrailers == std::string::npos)
					return;
				client.recvBuffer.erase(0, endTrailers + 4);
				client.request.bodyComplete = true;
				client.request.headers["Content-Length"] =
					sizeToStr(client.request.body.size());
				return;
			}

			client.chunkBytesRemaining = chunkSize;
			continue;
		}

		if (client.chunkBytesRemaining == CHUNK_EXPECT_CRLF) {
			if (client.recvBuffer.size() < 2)
				return;
			if (!(client.recvBuffer[0] == '\r' && client.recvBuffer[1] == '\n')) {
				generateErrorResponse(client, 400);
				return;
			}
			client.recvBuffer.erase(0, 2);
			client.chunkBytesRemaining = CHUNK_EXPECT_SIZE;
			continue;
		}

		if (client.chunkBytesRemaining > 0)
		{
			if (client.recvBuffer.empty())
				return;

			size_t take = client.recvBuffer.size();
			if (take > client.chunkBytesRemaining)
				take = client.chunkBytesRemaining;

			client.request.body.append(client.recvBuffer, 0, take);
			client.recvBuffer.erase(0, take);
			client.chunkBytesRemaining -= take;

			if (client.request.body.size() > maxBody) {
				generateErrorResponse(client, 413);
				return;
			}

			if (client.chunkBytesRemaining == 0) {
				client.chunkBytesRemaining = CHUNK_EXPECT_CRLF;
				continue;
			}
			return;
		}

		client.chunkBytesRemaining = CHUNK_EXPECT_SIZE;
	}
}

void TCPserver::ReadfromClient(Client &client)
{
	char buffer[4096];
	ssize_t bytes = recv(client.fd, buffer, sizeof(buffer), 0);

	if (bytes > 0)
		client.lastActivity = time(NULL);

	if (bytes <= 0)
	{
		if (bytes == 0)
			client.state = CLOSE_CONNECTION;
		return;
	}

	client.recvBuffer.append(buffer, static_cast<size_t>(bytes));

	if (!client.request.headersComplete) {
		size_t pos = client.recvBuffer.find("\r\n\r\n");
		if (pos == std::string::npos)
			return;

		client.request.headers.clear();
		client.request.method.clear();
		client.request.target.clear();
		client.request.version.clear();
		client.request.body.clear();
		client.request.bodyComplete = false;
		client.request.headersComplete = false;
		client.bodyReceived = 0;
		client.bodyExpected = static_cast<size_t>(-1);
		client.chunked = false;
		client.chunkBytesRemaining = 0;

		std::string headersPart = client.recvBuffer.substr(0, pos);
		parseRequestLineAndHeaders(client, headersPart);
		if (client.state == SEND_RESPONSE)
			return;

		client.recvBuffer.erase(0, pos + 4);
		client.request.headersComplete = true;

		if (!client.serverBlock)
			client.serverBlock = chooseServerBlock(client);
		if (!client.serverBlock) {
			generateErrorResponse(client, 500);
			return;
		}

		std::string pathOnly = getPathOnly(client.request.target);
		client.locationBlock = findLocationBlock(*client.serverBlock, pathOnly);
		if (!client.locationBlock)
			client.locationBlock = client.serverBlock;

		const std::string &method = client.request.method;
		const bool isPost = (method == "POST");

		if (client.chunked) {
			if (!isPost) {
				generateErrorResponse(client, 400);
				return;
			}
			readChunkedBody(client);
			if (client.state == SEND_RESPONSE)
				return;
			if (client.request.bodyComplete) {
				client.state = HANDLE_REQUEST;
				return;
			}
			return;
		}

		if (!isPost) {
			client.bodyExpected = 0;
			client.request.bodyComplete = true;
			client.recvBuffer.clear();
			client.state = HANDLE_REQUEST;
			return;
		}

		if (client.bodyExpected == static_cast<size_t>(-1)) {
			generateErrorResponse(client, 400);
			return;
		}

		size_t maxBody = getMaxBodySizeFromConfig(client.locationBlock, client.serverBlock);
		if (client.bodyExpected > maxBody)
		{
			generateErrorResponse(client, 413);
			return;
		}

		if (client.bodyExpected == 0) {
			client.request.bodyComplete = true;
			client.recvBuffer.clear();
			client.state = HANDLE_REQUEST;
			return;
		}
	}

	if (client.request.bodyComplete)
		return;

	if (client.chunked) {
		readChunkedBody(client);
		if (client.state == SEND_RESPONSE)
			return;
		if (client.request.bodyComplete) {
			client.state = HANDLE_REQUEST;
			return;
		}
		return;
	}

	if (client.bodyExpected == static_cast<size_t>(-1))
	{
		generateErrorResponse(client, 400);
		return;
	}

	size_t maxBody = getMaxBodySizeFromConfig(client.locationBlock, client.serverBlock);

	size_t remaining = 0;
	if (client.bodyExpected > client.bodyReceived)
		remaining = client.bodyExpected - client.bodyReceived;

	size_t take = client.recvBuffer.size();
	if (take > remaining)
		take = remaining;

	client.request.body.append(client.recvBuffer, 0, take);
	client.bodyReceived += take;
	client.recvBuffer.erase(0, take);

	if (client.request.body.size() > maxBody) {
		generateErrorResponse(client, 413);
		return;
	}

	if (client.bodyReceived >= client.bodyExpected) {
		client.request.bodyComplete = true;
		if (client.request.body.size() > client.bodyExpected)
			client.request.body.resize(client.bodyExpected);

		client.recvBuffer.clear();
		client.state = HANDLE_REQUEST;
		return;
	}
}
