#include <Server.hpp>


int main(int ac, char **av)
{
	std::string config_file = DEFAULT_CONF;
	if (ac > 1) config_file = av[1];
	try {
		Config conf = parse_config_file(config_file);
		std::vector<ServerConfig> servers = conf.getservers();
		Multiplexer *multi = Multiplexer::create_multiplexer(servers);
		// conf.debug();
		multi->run();
	} catch (std::runtime_error &e) {
		std::cerr << "[ Multiplexer::init ] " << e.what() << "\n";
		return (1);
	} catch (const char *e) {
		std::cerr << "[ Multiplexer::init ] " << e << "\n";
		return (1);
	}
}