#include <Server.hpp>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unistd.h>
#include <vector>

int main_cgi_test();
/*  int main(int ac, char **av)  */
int main()
{

	return main_cgi_test();
	/*  std::string config_file = DEFAULT_CONF;  */
	/**/
	/*  if (ac > 1) config_file = av[1];  */
	/*  try {  */
	/*  	Config conf = parse_config_file(config_file);  */
	/*  	std::vector<ServerConfig> servers = conf.getservers();  */
	/*  	Multiplexer *multi = Multiplexer::create_multiplexer(servers);  */
	/*  	conf.debug();  */
	/*  	multi->loop();  */
	/*  } catch (std::runtime_error &e) {  */
	/*  	std::cerr << "[ Multiplexer::init ] " << e.what() << "\n";  */
	/*  	return (1);  */
	/*  } catch (const char *e) {  */
	/*  	std::cerr << "[ Multiplexer::init ] " << e << "\n";  */
	/*  	return (1);  */
	/*  }  */
	/*     return (0);  */
	/*  return (0);  */
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
	request.headers["Content-Type"] = "text/plain";
	request.headers["REMOTE_ADDR"] = "127.0.0.1";

	cgi.setup(request_ref,
		   "./main.py",
		   "/bin/python3");

	/* 2) Cgi pipes and execution */
	cgi.execute();

	/* 3) How to write body anmd read cgi response*/
	write(cgi.streams[STDOUT_FILENO], body.c_str(), body.size());
	close(cgi.streams[STDOUT_FILENO]);

	/* 4) Parse Headers for the response. */
	std::memset(buffer, 0, sizeof(buffer));
	(cgi.state) = ReadingHeaders;
	while (read(cgi.streams[STDIN_FILENO], buffer, sizeof(buffer)))
	{
		/*  TODO: Look for `CRLF` or `NLNL` to mark the end of parsing the headers */
		/*  Keep on reading the response of the cgi until u hit end of headers. then parse the headers then try to get the body if the body exists.*/
		/*  std::cout << "Child sent: " << buffer;  */
		cgi.io_buffer += buffer;
		switch (cgi.state)
		{
			
			case WritingBody:
			case DONE:
			case Idle: {} break;        // Idk what is this for tho???
			case ReadingHeaders: {
				size_t position = cgi.io_buffer.find(CRLF);
				if (position == std::string::npos) position = cgi.io_buffer.find(NLNL);
				if (position != std::string::npos) {
					std::string tmp = cgi.io_buffer.substr(position, cgi.io_buffer.size());
					cgi.io_buffer
						.erase(position, cgi.io_buffer.size());
					cgi.state = ReadingBody;
					cgi.parse_headers();
					cgi.io_buffer.clear();
					body.swap(tmp);
				}
			} break;
			case ReadingBody: {
				body += buffer;
			} break;
		}
		std::memset(buffer, 0, sizeof(buffer));
	}
	std::cout << "body Sent from child: " << body;
	wait(NULL);
	return (0);
}
