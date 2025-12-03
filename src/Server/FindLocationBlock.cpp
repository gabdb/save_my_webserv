
#include "../../inc/Server/TCPserver.hpp"

// Renvoie la location la plus spécifique correspondant au path demandé.
// Si aucune location ne match, renvoie NULL (ou le serverBlock jsp encore)
const Block* TCPserver::findLocationBlock(const Block &serverBlock, const std::string &path)
{
    const Block *best = NULL;
    size_t bestLen = 0;

    // parcourir tous les sous-blocs du serverBlock
    for (size_t i = 0; i < serverBlock.location.size(); i++)
    {
        const Block *loc = &serverBlock.location[i];

        // Vérifier que c'est bien un bloc "location"
        if (loc->name != "location")
            continue;
        else if (loc->paths.empty())
            continue;

        const std::string &locPath = loc->paths[0]; // genre "/img"

        // vérifie si path commence par locPath, example: path == "/images/logo.png" et locPath == "/images"
        if (path.compare(0, locPath.size(), locPath) == 0)
        {
            // garder le match le plus long (le plus précis)
            if (locPath.size() > bestLen)
            {
                best = loc;
                bestLen = locPath.size();
            }
        }
    }
    return best; // peut être NULL → alors handleRequest utilise serverBlock pour appliquer root / index / etc.
}
