#pragma once

#include <map>
#include <string>

class Cgi {
private:
	std::string method;
	std::string uri;
	std::map<std::string, std::string> params;
	std::string filename;
	std::string extension;
	std::string executable;
};
