

// pseudo code

void TCPserver::handleRequest(Client &client)
{
    // ------------------------------------------
    // A) CHOISIR LE SERVER BLOCK (si pas encore fait)
    // ------------------------------------------
    if (client.serverBlock == NULL)
        client.serverBlock = chooseServerBlock(client);

    // Si quand même NULL => erreur config
    if (!client.serverBlock)
    {
        generateErrorResponse(client, 500, "Internal Server Error");
        return;
    }

    // ------------------------------------------
    // B) TROUVER LE BLOC LOCATION
    // ------------------------------------------
    client.locationBlock = findLocationBlock(*client.serverBlock, client.request.target);

    // Si aucune location spécifique ne match -> fallback = serverBlock
    if (!client.locationBlock)
        client.locationBlock = client.serverBlock;

    // ------------------------------------------
    // C) VERIFIER ALLOWED_METHODS
    // ------------------------------------------
    if (!isMethodAllowed(client))
    {
        generateErrorResponse(client, 405, "Method Not Allowed");
        return;
    }

    // ------------------------------------------
    // D) GERER REDIRECTION "return" (ex: 301)
    // ------------------------------------------
    if (hasReturnDirective(client))
    {
        handleReturnDirective(client);
        return;
    }

    // ------------------------------------------
    // E) GERER CGI (si extension match ou directive cgi_param)
    // ------------------------------------------
    if (shouldUseCGI(client))
    {
        handleCGI(client);
        return;
    }

    // ------------------------------------------
    // F) METHODES NORMALES
    // ------------------------------------------
    const std::string &method = client.request.method;

    if (method == "GET")
    {
        handleGET(client);
        return;
    }
    else if (method == "POST")
    {
        handlePOST(client);
        return;
    }
    else if (method == "DELETE")
    {
        handleDELETE(client);
        return;
    }

    // ------------------------------------------
    // G) SI AUCUNE METHODE GEREE -> 501
    // ------------------------------------------
    generateErrorResponse(client, 501, "Not Implemented");
}
