
#include "../../inc/Server/CGI.hpp"
#include "../../inc/Server/TCPserver.hpp"

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

void buildAndSendCgiResponse(Client &cl, const std::string &out)
{
    int status = 200;
    std::string contentType = "text/html";
    std::string extraHeaders;
    std::string body = out;

    size_t sep = out.find("\r\n\r\n");
    if (sep != std::string::npos) {
        std::string head = out.substr(0, sep);
        body = out.substr(sep + 4);

        std::istringstream iss(head);
        std::string line;
        while (std::getline(iss, line)) {
            if (!line.empty() && line[line.size() - 1] == '\r')
                line.erase(line.size() - 1);

            if (line.size() >= 7 && line.compare(0, 7, "Status:") == 0) {
                std::istringstream s(line.substr(7));
                s >> status;
            }
            else if (line.size() >= 13 && line.compare(0, 13, "Content-Type:") == 0) {
                std::string v = line.substr(13);
                while (!v.empty() && v[0] == ' ') v.erase(0, 1);
                if (!v.empty()) contentType = v;
            }
            else if (!line.empty()) {
                extraHeaders += line + "\r\n";
            }
        }
    }

    std::ostringstream oss;
    oss << "HTTP/1.1 " << status << " " << reasonPhrase(status) << "\r\n"
        << "Content-Length: " << body.size() << "\r\n"
        << "Content-Type: " << contentType << "\r\n"
        << "Connection: close\r\n"
        << extraHeaders
        << "\r\n";

    cl.sendBuffer = oss.str() + body;
    cl.wantWrite = true;
    cl.wantRead = false;
    cl.state = SEND_RESPONSE;
}
