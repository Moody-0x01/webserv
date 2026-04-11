#include <Server.hpp>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <unistd.h>

int main(int ac, char **av)
{
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

int main_cgi_test()
{
	Cgi cgi;
	HttpRequest request;
	char buffer[WRITE_CHUNK_SIZE];
	std::string body = "Hello from c++ test script:)\n";
	const HttpRequest &request_ref = request;

	/* 1) Cgi Setup */
	request.httpVersion = "HTTP/1.0";
	request.method = "GET";
	request.query_string = "book_shielf_id=72&book_id=69";
	request.content_length = 0;
	request.headers["content-type"] = "text/plain";
	request.headers["REMOTE_ADDR"] = "127.0.0.1";
	cgi.setup(request_ref,
		   "./main.py",
		   "/bin/python3");

	/* 2) Cgi pipes and execution */
	cgi.execute();

	/* 3) How to write body anmd read cgi response*/
	write(cgi.streams[STDOUT_FILENO], body.c_str(), body.size());
	close(cgi.streams[STDOUT_FILENO]);
	std::memset(buffer, 0, sizeof(buffer));
	while (read(cgi.streams[STDIN_FILENO], buffer, sizeof(buffer)))
	{
		std::cout << "Child sent: " << buffer;
		std::memset(buffer, 0, sizeof(buffer));
	}
	wait(NULL);
	return (0);
}
