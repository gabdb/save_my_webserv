
#include "../../inc/Server/TCPserver.hpp"

static std::string normalizeLoc(std::string loc)
{
	if (loc.empty())
		return "/";
	if (loc.size() > 1 && loc[loc.size() - 1] == '/')
		loc.erase(loc.size() - 1);
	return loc;
}

static std::string gab_joinPath(std::string a, const std::string &b)
{
	if (a.empty())
		return b;
	if (b.empty())
		return a;

	bool aSlash = (a[a.size() - 1] == '/');
	bool bSlash = (b[0] == '/');

	if (aSlash && bSlash)
		a.erase(a.size() - 1);
	else if (!aSlash && !bSlash)
		a += "/";

	return a + b;
}

std::string TCPserver::buildFullPath(const std::string &requestPath, const std::string &root, const std::string &locationPrefix) const {
	if (root.empty())
		return "";

	std::string path = requestPath;
	if (path.empty())
		path = "/";

	if (path.find("..") != std::string::npos)
		return "";

	std::string loc = normalizeLoc(locationPrefix);

	std::string remainder = path;

	if (loc != "/" && remainder.compare(0, loc.size(), loc) == 0)
	{
		if (remainder.size() == loc.size())
			remainder = "";
		else if (remainder[loc.size()] == '/')
			remainder = remainder.substr(loc.size());
	}

	return gab_joinPath(root, remainder);
}
