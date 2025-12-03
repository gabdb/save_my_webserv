
#include "../../inc/Server/TCPserver.hpp"
#include "../../inc/temp.hpp" // header miteux ?

bool keyContains(const key *k, const std::string &method)
{
    if (!k)
        return false;
    for (size_t i = 0; i < k->values.size(); ++i)
    {
        if (k->values[i] == method)
            return true;
    }
    return false;
}

// vérifie si la méthode HTTP (GET, POST, DELETE) est autorisée dans la config
bool TCPserver::isMethodAllowed(const Client &client) const
{
    const std::string &method = client.request.method;

    // vérifier dans la location (si elle existe)
    if (client.locationBlock)
    {
        const key *k = findKey(*client.locationBlock, "allowed_methods");
        if (k)
            return keyContains(k, method);
    }

    // sinon vérifier dans le serverBlock
    if (client.serverBlock)
    {
        const key *k = findKey(*client.serverBlock, "allowed_methods");
        if (k)
            return keyContains(k, method);
    }

    // si aucune directive allowed_methods -> default autoriser GET/POSTDELETE (basic)
    if (method == "GET" || method == "DELETE" || method == "POST")
        return true;

    return false;
}