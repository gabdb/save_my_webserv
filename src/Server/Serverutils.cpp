#include "../../inc/Server/TCPserver.hpp"
#include "../../inc/utils/utils.hpp"

std::string getRoot(const Client &client)
{
	const key *k = NULL;

	if (client.locationBlock) {
		k = findKey(*client.locationBlock, "root");
		if (k && !k->values.empty())
			return k->values[0];
	}
	if (client.serverBlock) {
		k = findKey(*client.serverBlock, "root");
		if (k && !k->values.empty())
			return k->values[0];
	}
	return "";
}

int ft_atoi(const char *str)
{
	int sign;
	long res;
	long buff;

	sign = 1;
	res = 0;
	buff = 0;
	while (*str == 32 || (*str >= 9 && *str <= 13))
		str++;
	if (*str == '-')
		sign *= (-1);
	if (*str == '-' || *str == '+')
		str++;
	while (*str >= '0' && *str <= '9') {
		res = res * 10 + *str++ - '0';
		if (buff > res && sign > 0)
			return (-1);
		else if (buff > res && sign < 0)
			return (0);
		buff = res;
	}
	return ((int)sign * res);
}

static bool parseSizeTDec(const std::string &s, size_t &out)
{
	if (s.empty())
		return false;
	size_t v = 0;
	for (size_t i = 0; i < s.size(); ++i) {
		if (s[i] < '0' || s[i] > '9')
			return false;
		size_t digit = static_cast<size_t>(s[i] - '0');
		if (v > (static_cast<size_t>(-1) - digit) / 10)
			return false;
		v = v * 10 + digit;
	}
	out = v;
	return true;
}

size_t getMaxBodySizeFromConfig(const Block *loc, const Block *srv)
{
	const size_t fallback = 1000000; // fallback 

	const key *k = NULL;
	size_t v = 0;

	if (loc) {
		k = findKey(*loc, "client_max_body_size");
		if (k && !k->values.empty()) {
			if (!parseSizeTDec(k->values[0], v) || v == 0)
				return fallback;
			return v;
		}
	}
	if (srv) {
		k = findKey(*srv, "client_max_body_size");
		if (k && !k->values.empty()) {
			if (!parseSizeTDec(k->values[0], v) || v == 0)
				return fallback;
			return v;
		}
	}
	return fallback;
}


std::string reasonPhrase(int status) {
    switch (status) {
    case 200:
        return "OK";
    case 201:
        return "Created";
    case 301:
        return "Moved Permanently";
    case 400:
        return "Bad Request";
    case 403:
        return "Forbidden";
    case 404:
        return "Not Found";
    case 405:
        return "Method Not Allowed";
    case 413:
        return "Payload Too Large";
    case 500:
        return "Internal Server Error";
    case 501:
        return "Not Implemented";
    default:
        return "Error";
    }
}

std::string trim(const std::string &s)
{
	size_t b = 0;
	while (b < s.size() && (s[b] == ' ' || s[b] == '\t'))
		++b;
	size_t e = s.size();
	while (e > b && (s[e - 1] == ' ' || s[e - 1] == '\t'))
		--e;
	return s.substr(b, e - b);
}


std::string toLower(std::string s)
{
	for (size_t i = 0; i < s.size(); ++i)
		s[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(s[i])));
	return s;
}

std::string sizeToStr(size_t n)
{
	std::ostringstream oss;
	oss << n;
	return oss.str();
}

std::string getPathOnly(const std::string &target)
{
	std::string pathOnly = target;
	size_t qpos = target.find('?');
	if (qpos != std::string::npos)
		pathOnly = target.substr(0, qpos);
	if (pathOnly.empty())
		pathOnly = "/";
	return pathOnly;
}
