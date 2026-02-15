#pragma once

# include <stdlib.h>
# include <vector>
# include <string>
# include <Parser/Config/Config.hpp>


class Parser {
private:
	const char *fn;
	size_t row; // Error reporting
	size_t col; // Error reporting
public:
	std::vector<std::string> read(void);
	Config parse(const char *file);
};
