
#include "../../inc/Server/CGI.hpp"
#include "../../inc/Server/TCPserver.hpp"
#include "../../inc/temp.hpp"

void TCPserver::buildAndSendCgiResponse(Client &cl, const std::string &out) {
    int status = 200;
    std::string contentType = "text/html";
    std::string extraHeaders;
    std::string body = out;

    // Checks for header in CGI
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
            } else if (line.size() >= 13 &&
                       line.compare(0, 13, "Content-Type:") == 0) {
                std::string v = line.substr(13);
                while (!v.empty() && v[0] == ' ')
                    v.erase(0, 1);
                if (!v.empty())
                    contentType = v;
            } else if (!line.empty()) {
                extraHeaders += line + "\r\n";
            }
        }
    }

    // Default (+ header defs if found)
    std::ostringstream oss;
    oss << "HTTP/1.1 " << status << " " << reasonPhrase(status) << "\r\n"
        << "Content-Length: " << body.size() << "\r\n"
        << "Content-Type: " << contentType << "\r\n"
        << "Connection: close\r\n"
        << extraHeaders << "\r\n";

    cl.sendBuffer = oss.str() + body;
    cl.state = SEND_RESPONSE;
}
