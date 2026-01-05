
#include "../../inc/Server/TCPserver.hpp"

std::string TCPserver::getRoot(const Client &client) const
{
    const key *k;

    k = findKey(*client.locationBlock, "root");
    if (k && !k->values.empty())
        return k->values[0];

    k = findKey(*client.serverBlock, "root");
    if (k && !k->values.empty())
        return k->values[0];

	//throw error ?
    return "";
}

static std::string normalizeLoc(std::string loc)
{
    if (loc.empty()) return "/";
    if (loc.size() > 1 && loc[loc.size() - 1] == '/')
        loc.erase(loc.size() - 1);
    return loc;
}

static std::string joinPath(std::string a, const std::string &b)
{
    if (a.empty()) return b;
    if (b.empty()) return a;

    bool aSlash = (a[a.size() - 1] == '/');
    bool bSlash = (b[0] == '/');

    if (aSlash && bSlash) a.erase(a.size() - 1);
    else if (!aSlash && !bSlash) a += "/";

    return a + b;
}

std::string TCPserver::buildFullPath(const std::string &requestPath,
                                     const std::string &root,
                                     const std::string &locationPrefix) const
{
    if (root.empty())
        return "";

    std::string path = requestPath;
    if (path.empty())
        path = "/";

    // sécurité: refuser les ".."
    if (path.find("..") != std::string::npos)
        return "";

    std::string loc = normalizeLoc(locationPrefix);

    // remainder = path sans préfixe de location
    std::string remainder = path;

    if (loc != "/" && remainder.compare(0, loc.size(), loc) == 0)
    {
        // on retire seulement si "/kapouet" ou "/kapouet/..."
        if (remainder.size() == loc.size())
            remainder = "";
        else if (remainder[loc.size()] == '/')
            remainder = remainder.substr(loc.size());
        // sinon "/kapouetX" => on ne touche pas
    }

    return joinPath(root, remainder);
}


/*
std::string TCPserver::buildFullPath(const std::string &requestPath, const std::string &root) const
{
    // 1) Sécurité inutile
    std::string path = requestPath;
    size_t qpos = path.find('?');
    if (qpos != std::string::npos)
        path = path.substr(0, qpos);

    // path vide -> "/"
    if (path.empty())
        path = "/";

    // bloquer path traversal simple (recommandé)
    if (path.find("..") != std::string::npos)
        return ""; // handleRequest traitera root vide => 500/403

    // enlever tous les '/' au début pour concat root + "/" + path proprement
    while (!path.empty() && path[0] == '/')
        path.erase(0, 1);

    // s’assurer que root finit par '/'
    std::string full = root;
    if (!full.empty() && full[full.size() - 1] != '/')
        full += "/";

    if (!path.empty())
        full += path;

    return full;
}
*/
