#include <Server.hpp>
#include <cstddef>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unistd.h>
#include <vector>

int main_cgi_test();
int main(int ac, char **av)
/*  int main()  */
{

	/*  return main_cgi_test();  */
	std::string config_file = DEFAULT_CONF;

	if (ac > 1) config_file = av[1];
	try {
		Config conf = parse_config_file(config_file);
		std::vector<ServerConfig> servers = conf.getservers();
		Multiplexer *multi = Multiplexer::create_multiplexer(servers);
		conf.debug();
		multi->loop();
	} catch (std::runtime_error &e) {
		std::cerr << "[ Multiplexer::init ] " << e.what() << "\n";
		return (1);
	} catch (const char *e) {
		std::cerr << "[ Multiplexer::init ] " << e << "\n";
		return (1);
	}
	return (0);
}
