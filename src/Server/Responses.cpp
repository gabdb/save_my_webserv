
#include "../../inc/Server/TCPserver.hpp"

static std::string toStr(size_t n)
{
    std::ostringstream oss;
    oss << n;
    return oss.str();
}

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

static void sendRawResponse(Client &client, int status,
                            const std::string &body,
                            const std::string &contentType,
                            const std::string &extraHeaders)
{
    std::string header =
        "HTTP/1.1 " + toStr((size_t)status) + " " + reasonPhrase(status) + "\r\n"
        "Content-Length: " + toStr(body.size()) + "\r\n"
        "Content-Type: " + contentType + "\r\n"
        "Connection: close\r\n" +
        extraHeaders +
        "\r\n";

    client.sendBuffer = header + body;
    client.wantWrite = true;
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

static std::string getRootSafe(const Client &client)
{
    const key *k = NULL;

    if (client.locationBlock) {
        k = findKey(*client.locationBlock, "root");
        if (k && !k->values.empty())
            return k->values[0];
    }
    if (client.serverBlock) {
        k = findKey(*client.serverBlock, "root");
        if (k && !k->values.empty())
            return k->values[0];
    }
    return "";
}

// error_page 404 /errors/404.html;
// error_page 500 502 503 /errors/50x.html;
static std::string findErrorPageValue(const Block &block, int status)
{
    std::ostringstream oss;
    oss << status;
    std::string statusStr = oss.str();

    for (size_t i = 0; i < block.keys.size(); ++i)
    {
        const key &k = block.keys[i];
        if (k.name != "error_page")
            continue;
        if (k.values.size() < 2)
            continue;

        std::string last = k.values[k.values.size() - 1]; // path (ou URI)
        for (size_t j = 0; j + 1 < k.values.size(); ++j)
        {
            if (k.values[j] == statusStr)
                return last;
        }
    }
    return "";
}

static std::string joinPath(std::string a, std::string b)
{
    if (a.empty()) return b;
    if (b.empty()) return a;

    bool aSlash = (a[a.size() - 1] == '/');
    bool bSlash = (b[0] == '/');

    if (aSlash && bSlash) a.erase(a.size() - 1);
    else if (!aSlash && !bSlash) a += "/";

    return a + b;
}

static std::string resolveErrorPageFsPath(const Client &client, const std::string &value)
{
    if (value.empty())
        return "";

    // si c'est déjà un path absolu vers un fichier existant, on le prend
    if (!value.empty() && value[0] == '/' && isRegularFile(value))
        return value;

    // sinon, on l'interprète comme un chemin "dans le root"
    std::string root = getRootSafe(client);
    if (root.empty())
        return "";

    std::string candidate = value;
    // value peut être "/errors/404.html"
    candidate = joinPath(root, candidate);

    if (isRegularFile(candidate))
        return candidate;

    return "";
}

static std::string basicErrorHtml(int status)
{
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

    // si on a une error page custom valide
    if (!fsPath.empty())
    {
        std::string body;
        if (readFileBinary(fsPath, body))
        {
            sendRawResponse(client, status, body, "text/html", "");
            return;
        }
    }

    // fallback
    sendRawResponse(client, status, basicErrorHtml(status), "text/html", "");
}
