#include <Server.hpp>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <sstream>
#include <unistd.h>

Cgi::~Cgi() {}
void Cgi::send_body_chunk(int conn) __THROWS_STRERROR
{
	::write(conn, this->io_buffer.c_str(), this->io_buffer.size());
	this->io_buffer.clear();
}

void Cgi::send_headers(int conn) __THROWS_STRERROR
{
	std::string headers = serialize_headers(this->headers, true);
	::write(conn, headers.c_str(), headers.size());
	this->headers_sent = true;
}

void Cgi::write() __THROWS_STRERROR
{
	if (this->state == WritingBody)
	{
		Multiplexer::write(this->streams[STDOUT_FILENO],
					this->io_buffer.c_str(),
					this->io_buffer.size());
		this->client_read_bytes += this->io_buffer.size();
		this->io_buffer.clear();
		if (this->client_content_length >= this->client_read_bytes)
			this->state = ReadingHeaders;
	}
}

void Cgi::read() __THROWS_STRERROR
{
	std::string tmp;
	char buffer[READ_CHUNK_SIZE];
	if (this->state == DONE)
		return ;

	ssize_t read_from_cgi = Multiplexer::read(this->streams[STDIN_FILENO],
				buffer,
				sizeof(buffer));
	this->io_buffer += buffer;
	switch (this->state)
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
			size_t position = this->io_buffer.find(CRLF);
			if (position == std::string::npos) position = this->io_buffer.find(NLNL);
			if (position != std::string::npos) {
				tmp = this->io_buffer.substr(position, this->io_buffer.size());
				this->io_buffer
					.erase(position, this->io_buffer.size());
				this->state = ReadingBody;
				this->parse_headers();
				this->io_buffer.swap(tmp);
			}
		} break;
		case ReadingBody: {
			this->cgi_read_bytes += read_from_cgi;
			if (this->cgi_read_bytes >= this->cgi_content_length)
				this->state = DONE;
		} break;
	}
}

void Cgi::action(uint32_t e) __THROWS_STRERROR
{
	if (e & (EPOLLERR | EPOLLHUP))
    {
        /*  this->free();  */
		// Note: in case of an error in the cgi, just cleanup resources here.
		// unregister from epoll whatever was registerred. then stop.
		return ;
    }

	try {
    if (e & EPOLLIN)
		this->read();
    if ((e & EPOLLOUT) || (e & EPOLLRDHUP))
		this->write();
	} catch (const char *e) {
		throw e;
	}
}

Cgi::Cgi()
{
	for (size_t i = 0; environ[i]; ++i) this->env.push_back(environ[i]);
	this->start_time          = 0;
	this->cgi_read_bytes      = 0;
	this->cgi_content_length      = 0;

	this->client_content_length = 0;
	this->client_read_bytes = 0;
	this->state               = Idle;
	this->io_buffer           = "";
}

void Cgi::setup(const HttpRequest &request, std::string fn, std::string interpreter_)
{
	std::stringstream stream;

	this->protocol = request.httpVersion;
	this->method = request.method;
	this->query_string = request.query_string;
	this->client_content_length = request.content_length;
	this->params = request.params;

	this->filename    = fn;
	this->executable  = fn;
	this->interpreter = interpreter_;
	this->gateway_interface = "CGI/1.1";
	if (request.headers.find("Content-Type") != request.headers.end())
		this->content_type = request.headers.at("Content-Type");
	else
		this->content_type = "application/octet-stream";
	stream << this->client_content_length;
	this->env.push_back("REQUEST_METHOD="+this->method);
	this->env.push_back("QUERY_STRING="+this->query_string);
	this->env.push_back("CONTENT_LENGTH="+stream.str());
	this->env.push_back("CONTENT_TYPE="+this->content_type);
	this->env.push_back("GATEWAY_INTERFAC="+this->gateway_interface);
	this->env.push_back("SCRIPT_NAME="+this->filename);
	this->env.push_back("PATH_TRANSLATED="+this->filename);
	this->env.push_back("REMOTE_ADDR="+request.headers.at("REMOTE_ADDR"));  // TODO: Get the ip of the client and forward it to the cgi.
	this->env.push_back("SERVER_PROTOCOL="+this->protocol);
}

static void close_fdlist(int fds[2])
{
	close(fds[STDIN_FILENO]);
	close(fds[STDOUT_FILENO]);
}

void Cgi::epoll_register(void) __THROWS_STRERROR
{
	struct epoll_event event;
	int reg;
	Multiplexer *self;

	self = Multiplexer::get_multiplexer(NULL);
	if (!self) throw "Well, failed to get a Multiplexer class";

	reg = this->streams[STDOUT_FILENO];
	event.events = EPOLLOUT | EPOLLRDHUP | EPOLLERR;
	this->state  = WritingBody;
	event.data.ptr = this;
	if (epoll_ctl(self->epoll_fd, EPOLL_CTL_ADD, reg, &event) == -1) throw strerror(errno);

	event.events = EPOLLIN | EPOLLRDHUP | EPOLLERR;
	reg = this->streams[STDIN_FILENO];
	if (this->method != "POST") this->state  = ReadingHeaders;
	if (epoll_ctl(self->epoll_fd, EPOLL_CTL_ADD, reg, &event) == -1) throw strerror(errno);
}

void Cgi::execute(void) __THROWS_STRERROR
{
	char *args[3] = {
		(char*)this->interpreter.c_str(),
		(char*)this->filename.c_str(), 
		NULL
	};
	int input[2];
	int output[2];

	std::vector<char*> envp;
    for (size_t i = 0; i < this->env.size(); ++i)
        envp.push_back((char*)this->env[i].c_str());
    envp.push_back(NULL);
	if (pipe(input) == -1) throw strerror(errno);
	if (pipe(output) == -1) {
		close_fdlist(input);
		throw strerror(errno);
	}
	this->pid = fork();
	if (this->pid == -1)
	{
		close_fdlist(output);
		close_fdlist(input);
		throw strerror(errno);
	}
	if (this->pid == 0)
	{
		dup2(input [STDIN_FILENO],  STDIN_FILENO);
		dup2(output[STDOUT_FILENO], STDOUT_FILENO);
		close_fdlist(output);
		close_fdlist(input);
		execve(this->interpreter.c_str(), args, &envp[0]);
		perror("execve");
		_exit(1);
	}
	// this->start_time = time(NULL); TODO: if some script hanged then idk maybe it is not required.
	this->streams[STDOUT_FILENO] = input[STDOUT_FILENO];
	close(input[STDIN_FILENO]);
	this->streams[STDIN_FILENO] = output[STDIN_FILENO];
	close(output[STDOUT_FILENO]);
	this->epoll_register();
}

void Cgi::parse_headers()    __THROWS_STRERROR
{
	std::vector<std::string> headers;
	std::vector<std::string> pair;

	headers =
		split(this->io_buffer, '\n');
	for (size_t i = 0; i < headers.size(); ++i)
	{
		pair = split(headers[i], ":");
		if (pair.size() != 2)
			throw "Invalid Head";
		this->headers[pair[0]] = pair[1];
		std::cout << pair[0] << " ---> " << pair[1] << "\n";
	}
	if (this->headers.find("Content-Length") == this->headers.end())
		throw "emmm no content length was given from cgi";

	std::stringstream ss(this->headers.at("Content-Length"));
	ss << this->cgi_content_length;
}
