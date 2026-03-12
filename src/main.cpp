# include <Server.hpp>
#include <exception>
#include <iostream>
#include <stdexcept>

int main(int ac, char **av)
{
	std::string config_file = DEFAULT_CONF;

	if (ac > 1) config_file = av[1];
	try {
		Config conf = parse_config_file(config_file);
		std::vector<ServerConfig> servers = conf.getservers();
		Multiplexer::init(servers);
		conf.debug();
		Multiplexer::loop();
		Multiplexer::deinit();
	} catch (std::runtime_error &e) {
		std::cerr << "[ Multiplexer::init ] " << e.what() << "\n";
		return (1);
	}
    return (0);
}
