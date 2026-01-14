
#include "../../inc/Server/TCPserver.hpp"

static std::string toStr(size_t n)
{
	std::ostringstream oss;
	oss << n;
	return oss.str();
}

static void sendRawResponse(Client &client, int status, const std::string &body,
							const std::string &contentType,
							const std::string &extraHeaders) {
		std::string header = "HTTP/1.1 " + toStr((size_t)status) + " " +
							reasonPhrase(status) +
							"\r\n"
							"Content-Length: " +
							toStr(body.size()) +
							"\r\n"
							"Content-Type: " +
							contentType +
							"\r\n"
							"Connection: close\r\n" +
							extraHeaders + "\r\n";

	client.sendBuffer = header + body;
	client.state = SEND_RESPONSE;
}

static bool isRegularFile(const std::string &path)
{
    struct stat st;
    if (stat(path.c_str(), &st) != 0)
        return false;
    return S_ISREG(st.st_mode);
}

static bool readFileBinary(const std::string &path, std::string &out)
{
    std::ifstream f(path.c_str(), std::ios::in | std::ios::binary);
    if (!f)
        return false;
    std::ostringstream oss;
    oss << f.rdbuf();
    out = oss.str();
    return true;
}

static std::string findErrorPageValue(const Block &block, int status)
{
    std::ostringstream oss;
    oss << status;
    std::string statusStr = oss.str();

    for (size_t i = 0; i < block.keys.size(); ++i) {
        const key &k = block.keys[i];
        if (k.name != "error_page")
            continue;
        if (k.values.size() < 2)
            continue;

        std::string last = k.values[k.values.size() - 1]; // path (ou URI)
        for (size_t j = 0; j + 1 < k.values.size(); ++j) {
            if (k.values[j] == statusStr)
                return last;
        }
    }
    return "";
}

static std::string gab_joinPath(std::string a, std::string b) {
    if (a.empty())
        return b;
    if (b.empty())
        return a;

    bool aSlash = (a[a.size() - 1] == '/');
    bool bSlash = (b[0] == '/');

    if (aSlash && bSlash)
        a.erase(a.size() - 1);
    else if (!aSlash && !bSlash)
        a += "/";

    return a + b;
}

static std::string resolveErrorPageFsPath(const Client &client, const std::string &value)
{
	if (value.empty())
		return "";

	if (!value.empty() && value[0] == '/' && isRegularFile(value))
		return value;

	std::string root = getRoot(client);
	if (root.empty())
		return "";

	std::string candidate = value;
	candidate = gab_joinPath(root, candidate);

	if (isRegularFile(candidate))
		return candidate;

	return "";
}

static std::string basicErrorHtml(int status) {
	std::string phrase = reasonPhrase(status);
	std::ostringstream oss;
	oss << "<html><head><title>" << status << " " << phrase << "</title></head>"
		<< "<body><h1>" << status << " " << phrase << "</h1></body></html>";
	return oss.str();
}

void TCPserver::generateErrorResponse(Client &client, int status)
{
	std::string errorValue;

	if (client.locationBlock)
		errorValue = findErrorPageValue(*client.locationBlock, status);
	if (errorValue.empty() && client.serverBlock)
		errorValue = findErrorPageValue(*client.serverBlock, status);

	std::string fsPath = resolveErrorPageFsPath(client, errorValue);

	if (!fsPath.empty()) {
		std::string body;
		if (readFileBinary(fsPath, body)) {
			sendRawResponse(client, status, body, "text/html", "");
			return;
		}
	}

	// fallback
	sendRawResponse(client, status, basicErrorHtml(status), "text/html", "");
}
