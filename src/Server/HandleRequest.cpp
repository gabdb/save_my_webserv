
#include "../../inc/Server/TCPserver.hpp"
#include "../../inc/utils/utils.hpp"

static std::string reasonPhrase(int status)
{
	switch (status) {
		case 200: return "OK";
		case 201: return "Created";
		case 301: return "Moved Permanently";
		case 400: return "Bad Request";
		case 403: return "Forbidden";
		case 404: return "Not Found";
		case 405: return "Method Not Allowed";
		case 413: return "Payload Too Large";
		case 500: return "Internal Server Error";
		case 501: return "Not Implemented";
		default:  return "Error";
	}
}

static bool isAutoindexOn(const Block *loc, const Block *srv)
{
	const key *k = NULL;
	if (loc) {
		k = findKey(*loc, "autoindex");
		if (k && !k->values.empty())
			return (k->values[0] == "on" || k->values[0] == "true" || k->values[0] == "1");
	}
	if (srv) {
		k = findKey(*srv, "autoindex");
		if (k && !k->values.empty())
			return (k->values[0] == "on" || k->values[0] == "true" || k->values[0] == "1");
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

static void sendSimpleResponse(Client &client, int status, const std::string &body,
								const std::string &contentType,
								const std::string &extraHeaders)
{
	std::string header =
		"HTTP/1.1 " + intToStr(status) + " " + reasonPhrase(status) + "\r\n"
		"Content-Length: " + intToStr((int)body.size()) + "\r\n"
		"Content-Type: " + contentType + "\r\n"
		"Connection: close\r\n" +
		extraHeaders +
		"\r\n";

	client.sendBuffer = header + body;
	client.wantWrite = true;
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
	for (size_t i = 0; i < s.size(); ++i)
	{
		if (s[i] == '&') out += "&amp;";
		else if (s[i] == '<') out += "&lt;";
		else if (s[i] == '>') out += "&gt;";
		else if (s[i] == '"') out += "&quot;";
		else out += s[i];
	}
	return out;
}

static void generateAutoindex(Client &client, const std::string &fsDirPath, const std::string &urlPathWithSlash)
{
	DIR *dir = opendir(fsDirPath.c_str());
	if (!dir)
	{
		sendSimpleResponse(client, 500,
			"<html><body><h1>500 Internal Server Error</h1></body></html>",
			"text/html", "");
		return;
	}

	std::vector<std::string> entries;
	struct dirent *ent;
	while ((ent = readdir(dir)) != NULL)
	{
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

	// Parent link (optionnel mais pratique)
	//if (urlPathWithSlash != "/")
	//    body += "<li><a href=\"../\">../</a></li>";

	for (size_t i = 0; i < entries.size(); ++i)
	{
		std::string name = entries[i];
		std::string href = urlPathWithSlash + name;

		// Si c’est un dossier, ajouter "/"
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
	if (s.size() < suffix.size()) return false;
	return s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0;
}

static std::string pickCgiInterpreter(const std::string &fsPath)
{
	// extension-based CGI
	if (endsWith(fsPath, ".php")) return "/usr/bin/php-cgi";
	if (endsWith(fsPath, ".py"))  return "/usr/bin/python3";
	if (endsWith(fsPath, ".pl"))  return "/usr/bin/perl";
	if (endsWith(fsPath, ".sh"))  return "/bin/bash";
	return "";
}

void TCPserver::handleRequest(Client &client)
{
	// split target -> path + query
	std::string target = client.request.target;
	std::string pathOnly = target;
	std::string queryString;

	size_t qpos = target.find('?');
	if (qpos != std::string::npos)
	{
		pathOnly = target.substr(0, qpos);
		queryString = target.substr(qpos + 1);
	}
	if (pathOnly.empty())
		pathOnly = "/";

	// choisir server + location
	if (!client.serverBlock)
		client.serverBlock = chooseServerBlock(client);

	if (!client.serverBlock)
	{
		generateErrorResponse(client, 500);
		return;
	}

	client.locationBlock = findLocationBlock(*client.serverBlock, pathOnly);
	if (!client.locationBlock)
		client.locationBlock = client.serverBlock;

	// méthode autorisée par la config ?
	if (!isMethodAllowed(client))
	{
		generateErrorResponse(client, 405);
		return;
	}

	// root + fullPath filesystem
	std::string root = getRoot(client);
	if (root.empty())
	{
		generateErrorResponse(client, 500);
		return;
	}

	std::string locPrefix = "/";
	if (client.locationBlock && !client.locationBlock->paths.empty())
		locPrefix = client.locationBlock->paths[0];

	std::string fullPath = buildFullPath(pathOnly, root, locPrefix);
	if (fullPath.empty())
	{
		generateErrorResponse(client, 403); // traversal/forbidden
		return;
	}

	const std::string &method = client.request.method;

	// CGI : ici le script DOIT exister
	std::string interpreter = pickCgiInterpreter(fullPath);
	if (!interpreter.empty())
	{
		if (method != "GET" && method != "POST")
		{
			generateErrorResponse(client, 405);
			return;
		}

		struct stat stScript;
		if (stat(fullPath.c_str(), &stScript) < 0 || S_ISDIR(stScript.st_mode))
		{
			generateErrorResponse(client, 404);
			return;
		}

		try
		{
			startCgiForClient(client, interpreter, fullPath, queryString);
			client.wantRead = false;
			client.wantWrite = false;
			client.state = WAIT_CGI;
			client.lastActivity = time(NULL);
			return;
		}
		catch (...)
		{
			generateErrorResponse(client, 500);
			return;
		}
	}

	// POST non-CGI : peut créer un nouveau fichier => pas de 404 ici
	if (method == "POST")
	{
		// si URL finit par '/', on refuse (POST sur directory)
		if (!pathOnly.empty() && pathOnly[pathOnly.size() - 1] == '/')
		{
			generateErrorResponse(client, 403);
			return;
		}

		// si ça existe ET que c'est un dossier -> 403 (sinon OK)
		struct stat stPost;
		if (stat(fullPath.c_str(), &stPost) == 0 && S_ISDIR(stPost.st_mode))
		{
			generateErrorResponse(client, 403);
			return;
		}

		handlePOST(client, fullPath);
		return;
	}

	// GET / DELETE : doivent pointer vers quelque chose d'existant
	struct stat st;
	if (stat(fullPath.c_str(), &st) < 0)
	{
		generateErrorResponse(client, 404);
		return;
	}

	// Directory logic (redir + index + autoindex) : GET only
	if (S_ISDIR(st.st_mode))
	{
		// si pas de slash final : on redirige seulement pour GET
		if (!pathOnly.empty() && pathOnly[pathOnly.size() - 1] != '/')
		{
			if (method == "GET")
			{
				std::string location = pathOnly + "/";
				if (!queryString.empty())
					location += "?" + queryString;
				sendRedirect(client, location);
				return;
			}
			generateErrorResponse(client, 403);
			return;
		}

		// index
		std::string indexFile = getIndexFile(client.locationBlock, client.serverBlock);
		std::string indexPath = fullPath;
		if (!indexPath.empty() && indexPath[indexPath.size() - 1] != '/')
			indexPath += "/";
		indexPath += indexFile;

		struct stat stIndex;
		if (stat(indexPath.c_str(), &stIndex) == 0 && !S_ISDIR(stIndex.st_mode))
		{
			if (method == "GET")
			{
				handleGET(client, indexPath);
				return;
			}
			generateErrorResponse(client, 403);
			return;
		}

		// autoindex seulement pour GET
		if (method == "GET" && isAutoindexOn(client.locationBlock, client.serverBlock))
		{
			generateAutoindex(client, fullPath, pathOnly);
			return;
		}

		generateErrorResponse(client, 403);
		return;
	}

	// Méthodes finales
	if (method == "GET")
	{
		handleGET(client, fullPath);
		return;
	}
	else if (method == "DELETE")
	{
		handleDELETE(client, fullPath);
		return;
	}

	// Si une méthode inconnue arrive ici (normalement filtrée avant)
	generateErrorResponse(client, 501);
}


/*
void TCPserver::handleRequest(Client &client)
{
    // splitter target -> path + query
    std::string target = client.request.target;
    std::string pathOnly = target;
    std::string queryString;

    size_t check = target.find('?');
    if (check != std::string::npos) {
        pathOnly = target.substr(0, check);
        queryString = target.substr(check + 1);
    }
    if (pathOnly.empty())
        pathOnly = "/";

    // choisir server block
    if (!client.serverBlock)
        client.serverBlock = chooseServerBlock(client);
    if (!client.serverBlock) {
        generateErrorResponse(client, 500);
        return;
    }

    // sous-bloc location
    client.locationBlock = findLocationBlock(*client.serverBlock, pathOnly);
    if (!client.locationBlock)
        client.locationBlock = client.serverBlock;

    // méthode autorisée ?
    if (!isMethodAllowed(client)) {
        generateErrorResponse(client, 405);
        return;
    }

    // root
    std::string root = getRoot(client);
    if (root.empty()) {
        generateErrorResponse(client, 500);
        return;
    }

	std::string locPrefix = "/";
	if (client.locationBlock && !client.locationBlock->paths.empty())
		locPrefix = client.locationBlock->paths[0];

	std::string fullPath = buildFullPath(pathOnly, root, locPrefix);
	if (fullPath.empty())
	{
		generateErrorResponse(client, 403); // accès interdit / traversal
		return;
	}


    // stat()
    struct stat st;
    if (stat(fullPath.c_str(), &st) < 0) {
        generateErrorResponse(client, 404);
        return;
    }

    // 7) Si directory: redirect + index/autoindex
    if (S_ISDIR(st.st_mode))
    {
        // 7.1) Si URL ne finit pas par '/', on redirect (comportement “browser-friendly”)
        if (pathOnly[pathOnly.size() - 1] != '/')
        {
            std::string location = pathOnly + "/";
            if (!queryString.empty())
                location += "?" + queryString;
            sendRedirect(client, location);
            return;
        }

        // 7.2) Tenter index
        std::string indexFile = getIndexFile(client.locationBlock, client.serverBlock);

        std::string indexPath = fullPath;
        if (!indexPath.empty() && indexPath[indexPath.size() - 1] != '/')
            indexPath += "/";
        indexPath += indexFile;

        struct stat stIndex;
        if (stat(indexPath.c_str(), &stIndex) == 0 && !S_ISDIR(stIndex.st_mode))
        {
            // servir le fichier index via GET (binaire + content-type -> handleGET amélioré)
            if (client.request.method == "GET") {
                handleGET(client, indexPath);
                return;
            }
            // si quelqu’un fait POST/DELETE sur un directory
            generateErrorResponse(client, 403);
            return;
        }

        // sinon autoindex si activé
        if (isAutoindexOn(client.locationBlock, client.serverBlock))
        {
            generateAutoindex(client, fullPath, pathOnly);
            return;
        }

        generateErrorResponse(client, 403);
        return;
    }

    // CGI (based on extension)
    std::string interpreter = pickCgiInterpreter(fullPath);
    if (!interpreter.empty())
    {
        // CGI pour GET/POST
        if (client.request.method != "GET" && client.request.method != "POST") {
            generateErrorResponse(client, 405);
            return;
        }

        try {
            startCgiForClient(client, interpreter, fullPath, queryString);
            // on stoppe le traitement "client socket"
            // le CGI sera drivé par poll() via ses pipes
            client.wantRead = false;
            client.wantWrite = false;
            client.state = WAIT_CGI;
            client.lastActivity = time(NULL);
            // question: libérer mémoire ??
            // client.request.body.clear();

            return;
        } catch (...) {
            generateErrorResponse(client, 500);
            return;
        }
    }

    // choisir méthode
    const std::string &method = client.request.method;

    if (method == "GET") {
        handleGET(client, fullPath);
        return;
    }
    else if (method == "POST") {
        handlePOST(client, fullPath);
        return;
    }
    else if (method == "DELETE") {
        handleDELETE(client, fullPath);
        return;
    }

    generateErrorResponse(client, 501);
}
*/
