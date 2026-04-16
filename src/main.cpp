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

int main_cgi_test()
{
	Cgi cgi;
	HttpRequest request;
	ssize_t read_from_cgi;
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
		   "./www/cgi-bin/getvideo.py",
		   "/bin/python3");

	/* 2) Cgi pipes and execution */
	cgi.execute();

	/* 3) How to write body anmd read cgi response*/
	/*  write(cgi.streams[STDOUT_FILENO], body.c_str(), body.size());  */
	/*  close(cgi.streams[STDOUT_FILENO]);  */

	/* 4) Parse Headers for the response. */
	std::memset(buffer, 0, WRITE_CHUNK_SIZE);
	(cgi.state) = ReadingHeaders;
	while (true)
	{
		read_from_cgi = read(cgi.streams[STDIN_FILENO], buffer, WRITE_CHUNK_SIZE);
		if (read_from_cgi == 0 || read_from_cgi == -1) break ;
		switch (cgi.state)
		{
			case DONE: {} break;
			case Idle: {
				std::cout << "Wtf bro this should be done in write\n";
				abort();
			} break;        // Idk what is this for tho???
			case WritingBody: {
				std::cout << "Wtf bro this should be done in write\n";
				abort();
			} break; // this is done somewhere else??
			case ReadingHeaders: {
				cgi.append_into_headers_buffer(buffer, read_from_cgi);
				if (cgi.find_header_end() != cgi.headers_buffer.end())
					cgi.state = ReadingBody;
			} break;
			case ReadingBody: {
				cgi.append_into_body_buffer(buffer, read_from_cgi);
			} break;
		}
		std::memset(buffer, 0, sizeof(buffer));
	}
	print_buffer(cgi.headers_buffer, "Headers: ");
	print_buffer(cgi.body_buffer, "Body: ");
	wait(NULL);
	return (0);
}
