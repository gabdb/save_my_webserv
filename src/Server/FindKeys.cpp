
#include "../../inc/config/Config.hpp"
#include "../../inc/Server/TCPserver.hpp"

const key *findKey(const Block &block, const std::string &name)
{
	for (size_t i = 0; i < block.keys.size(); ++i)
	{
		if (block.keys[i].name == name)
			return &block.keys[i];
	}
	return NULL;
}


// jsp trop quand celle-la sera utile 
//std::vector<const key *> findAllKeys(const Block &block, const std::string &name); // TODO:
