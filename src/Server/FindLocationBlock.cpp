
#include "../../inc/Server/TCPserver.hpp"

const Block *TCPserver::findLocationBlock(const Block &serverBlock, const std::string &path)
{
	const Block *best = NULL;
	size_t bestLen = 0;

	for (size_t i = 0; i < serverBlock.locations.size(); i++)
	{
		const Block *loc = &serverBlock.locations[i];

		if (loc->name != "location")
			continue;
		else if (loc->paths.empty())
			continue;

		const std::string &locPath = loc->paths[0];

		if (path.compare(0, locPath.size(), locPath) == 0)
		{
			if (locPath.size() > bestLen) {
				best = loc;
				bestLen = locPath.size();
			}
		}
	}
	return best;
}
