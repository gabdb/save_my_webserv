
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

void TCPserver::handleDELETE(Client &client, const std::string &path)
{
    struct stat st;

    if (stat(path.c_str(), &st) < 0) {
        generateErrorResponse(client, 404);
        return;
    }

    // refuser suppression de dossiers
    if (S_ISDIR(st.st_mode)) {
        generateErrorResponse(client, 403);
        return;
    }

    if (remove(path.c_str()) != 0) {
        generateErrorResponse(client, 500);
        return;
    }

    sendHtml(client, 200, "<html><body><h1>Deleted</h1></body></html>");
}

