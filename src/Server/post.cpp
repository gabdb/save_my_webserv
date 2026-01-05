
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

static void sendHtml(Client &client, int status, const std::string &body)
{
    std::string header =
        "HTTP/1.1 " + intToStr(status) + " " + reasonPhrase(status) + "\r\n"
        "Content-Length: " + intToStr((int)body.size()) + "\r\n"
        "Content-Type: text/html\r\n"
        "Connection: close\r\n\r\n";

    client.sendBuffer = header + body;
    client.wantWrite = true;
    client.state = SEND_RESPONSE;
}

void TCPserver::handlePOST(Client &client, const std::string &path)
{
    // autoriser body vide => crée un fichier vide (c’est OK)
    // si tu veux forcer un body non-vide, remets ton check, mais c’est rarement utile.

    // vérifier que le dossier parent existe
    size_t slash = path.find_last_of('/');
    if (slash == std::string::npos) {
        generateErrorResponse(client, 400);
        return;
    }

    std::string dir = path.substr(0, slash);
    struct stat st;

    if (dir.empty()) dir = "/";

    if (stat(dir.c_str(), &st) < 0 || !S_ISDIR(st.st_mode)) {
        generateErrorResponse(client, 404);
        return;
    }

    // créer/écraser le fichier
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
