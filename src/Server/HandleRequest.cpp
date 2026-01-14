
#include "../../inc/Server/TCPserver.hpp"
#include "../../inc/config/Config.hpp"
#include "../../inc/utils/utils.hpp"

static bool isAutoindexOn(const Block *loc, const Block *srv)
{
	const key *k = NULL;
	if (loc) {
		k = findKey(*loc, "autoindex");
		if (k && !k->values.empty())
			return (k->values[0] == "on" || k->values[0] == "true" ||
					k->values[0] == "1");
	}
	if (srv) {
		k = findKey(*srv, "autoindex");
		if (k && !k->values.empty())
			return (k->values[0] == "on" || k->values[0] == "true" ||
					k->values[0] == "1");
	}
	return false;
}

static std::string getIndexFile(const Block *loc, const Block *srv)
{
	const key *k = NULL;

	if (loc) {
		k = findKey(*loc, "index");
		if (k && !k->values.empty())
			return k->values[0];
	}
	if (srv) {
		k = findKey(*srv, "index");
		if (k && !k->values.empty())
			return k->values[0];
	}
	// fallback
	return "index.html";
}

static void sendSimpleResponse(Client &client, int status, const std::string &body, const std::string &contentType, const std::string &extraHeaders)
{
	std::string header = "HTTP/1.1 " + intToStr(status) + " " +
							reasonPhrase(status) +
							"\r\n"
							"Content-Length: " +
							intToStr((int)body.size()) +
							"\r\n"
							"Content-Type: " +
							contentType +
							"\r\n"
							"Connection: close\r\n" +
							extraHeaders + "\r\n";

	client.sendBuffer = header + body;
	client.state = SEND_RESPONSE;
}

static void sendRedirect(Client &client, const std::string &location)
{
    std::string body = "<html><body>Moved to <a href=\"" + location + "\">" + location + "</a></body></html>";
    std::string extra = "Location: " + location + "\r\n";
    sendSimpleResponse(client, 301, body, "text/html", extra);
}

static std::string htmlEscape(const std::string &s)
{
	std::string out;
	for (size_t i = 0; i < s.size(); ++i) {
		if (s[i] == '&')
			out += "&amp;";
		else if (s[i] == '<')
			out += "&lt;";
		else if (s[i] == '>')
			out += "&gt;";
		else if (s[i] == '"')
			out += "&quot;";
		else
			out += s[i];
	}
	return out;
}

static void generateAutoindex(Client &client, const std::string &fsDirPath, const std::string &urlPathWithSlash)
{
    DIR *dir = opendir(fsDirPath.c_str());
    if (!dir) {
        sendSimpleResponse(
            client, 500,
            "<html><body><h1>500 Internal Server Error</h1></body></html>",
            "text/html", "");
        return;
    }

    std::vector<std::string> entries;
    struct dirent *ent;
    while ((ent = readdir(dir)) != NULL) {
        std::string name = ent->d_name;
        if (name == ".")
            continue;
        entries.push_back(name);
    }
    closedir(dir);

    std::sort(entries.begin(), entries.end());

    std::string body;
    body += "<html><head><title>Index of " + htmlEscape(urlPathWithSlash) + "</title></head><body>";
    body += "<h1>Index of " + htmlEscape(urlPathWithSlash) + "</h1><ul>";

    for (size_t i = 0; i < entries.size(); ++i) {
        std::string name = entries[i];
        std::string href = urlPathWithSlash + name;

        struct stat st;
        std::string full = fsDirPath;
        if (!full.empty() && full[full.size() - 1] != '/')
            full += "/";
        full += name;

        if (stat(full.c_str(), &st) == 0 && S_ISDIR(st.st_mode))
            href += "/";

        body += "<li><a href=\"" + htmlEscape(href) + "\">" + htmlEscape(name) + "</a></li>";
    }

    body += "</ul></body></html>";

    sendSimpleResponse(client, 200, body, "text/html", "");
}

static bool endsWith(const std::string &s, const std::string &suffix)
{
	if (s.size() < suffix.size())
		return false;
	return (s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0);
}

static std::string pickCgiInterpreter(const std::string &fsPath)
{
	if (endsWith(fsPath, ".php"))
		return "/usr/bin/php-cgi";
	if (endsWith(fsPath, ".py"))
		return "/usr/bin/python3";
	if (endsWith(fsPath, ".pl"))
		return "/usr/bin/perl";
	if (endsWith(fsPath, ".sh"))
		return "/bin/bash";
	if (endsWith(fsPath, ".go"))
		return "/usr/bin/go";
	return "";
}

void TCPserver::handleRequest(Client &client)
{
	std::string target = client.request.target;
	std::string pathOnly = target;
	std::string queryString;

	size_t qpos = target.find('?');
	if (qpos != std::string::npos) {
		pathOnly = target.substr(0, qpos);
		queryString = target.substr(qpos + 1);
	}
	if (pathOnly.empty())
		pathOnly = "/";

	if (!client.serverBlock)
		client.serverBlock = chooseServerBlock(client);
	if (!client.serverBlock) {
		generateErrorResponse(client, 500);
		return;
	}

	client.locationBlock = findLocationBlock(*client.serverBlock, pathOnly);
	if (!client.locationBlock) {
		client.locationBlock = client.serverBlock;
	}

	if (client.request.method == "POST" && !client.chunked && client.bodyExpected != static_cast<size_t>(-1))
	{
		size_t maxBody = getMaxBodySizeFromConfig(client.locationBlock, client.serverBlock);
		if (client.bodyExpected > maxBody)
		{
			generateErrorResponse(client, 413);
			return;
		}
	}

	if (!isMethodAllowed(client)) {
		generateErrorResponse(client, 405);
		return;
	}

	std::string root = getRoot(client);
	if (root.empty()) {
		generateErrorResponse(client, 500);
		return;
	}

	std::string locPrefix = "/";
	if (client.locationBlock && !client.locationBlock->paths.empty())
		locPrefix = client.locationBlock->paths[0];

	std::string fullPath = buildFullPath(pathOnly, root, locPrefix);
	if (fullPath.empty()) {
		generateErrorResponse(client, 403);
		return;
	}

	const std::string &method = client.request.method;

	const key *retKey = findKey(*client.locationBlock, "return");
	if (retKey) {
		if (retKey->values.size() < 2) {
			generateErrorResponse(client, 500);
			return;
		}
		const std::string &code = retKey->values[0];
		const std::string &redirTarget = retKey->values[1];
		if (code != "301") {
			generateErrorResponse(client, 500);
			return;
		}
		sendRedirect(client, redirTarget);
		return;
	}

	std::string interpreter = pickCgiInterpreter(fullPath);
	if (!interpreter.empty()) {
		if (method != "GET" && method != "POST") {
			generateErrorResponse(client, 405);
			return;
		}

		struct stat stScript;
		if (stat(fullPath.c_str(), &stScript) < 0 ||
			S_ISDIR(stScript.st_mode) || !S_ISREG(stScript.st_mode)) {
			generateErrorResponse(client, 404);
			return;
		}

		try {
			startCgiForClient(client, interpreter, fullPath, queryString);
			client.lastActivity = time(NULL);
			return;
		} catch (...) {
			generateErrorResponse(client, 500);
			return;
		}
	}

	if (method == "POST")
	{
		if (!pathOnly.empty() && pathOnly[pathOnly.size() - 1] == '/') {
			generateErrorResponse(client, 403);
			return;
		}

		struct stat stPost;
		if (stat(fullPath.c_str(), &stPost) == 0 && S_ISDIR(stPost.st_mode)) {
			generateErrorResponse(client, 403);
			return;
		}

		handlePOST(client, fullPath);
		return;
	}

	struct stat st;
	if (stat(fullPath.c_str(), &st) < 0) {
		generateErrorResponse(client, 404);
		return;
	}

	if (S_ISDIR(st.st_mode))
	{
		if (!pathOnly.empty() && pathOnly[pathOnly.size() - 1] != '/') {
			if (method == "GET") {
				std::string location = pathOnly + "/";
				if (!queryString.empty())
					location += "?" + queryString;
				sendRedirect(client, location);
				return;
			}
			generateErrorResponse(client, 403);
			return;
		}

		std::string indexFile =
			getIndexFile(client.locationBlock, client.serverBlock);
		std::string indexPath = fullPath;
		if (!indexPath.empty() && indexPath[indexPath.size() - 1] != '/')
			indexPath += "/";
		indexPath += indexFile;

		struct stat stIndex;
		if (stat(indexPath.c_str(), &stIndex) == 0 &&
			!S_ISDIR(stIndex.st_mode)) {
			if (method == "GET") {
				handleGET(client, indexPath);
				return;
			}
			generateErrorResponse(client, 403);
			return;
		}

		if (method == "GET" &&
			isAutoindexOn(client.locationBlock, client.serverBlock)) {
			generateAutoindex(client, fullPath, pathOnly);
			return;
		}

		generateErrorResponse(client, 403);
		return;
	}

	if (!S_ISREG(st.st_mode)) {
		generateErrorResponse(client, 404);
		return;
	}

	if (method == "GET") {
		handleGET(client, fullPath);
		return;
	} else if (method == "DELETE") {
		handleDELETE(client, fullPath);
		return;
	}

	generateErrorResponse(client, 501);
}
