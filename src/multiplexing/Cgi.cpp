#include <Server.hpp>
#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cstddef>
#include <stdint.h>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <iostream>
#include <ostream>
#include <sstream>
#include <string>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

bool Cgi::did_fail(void) const
{
	return (this->gateway_failed);
}

Cgi::~Cgi()
{
	std::cout << "Cgi~\n";
	this->done();
}

void Cgi::close_write(void) {
	std::cout << "close_write\n";
	unregister_fd(this->streams[CGI_WRITE_END]);
	this->streams[CGI_WRITE_END] = -1;
	if (this->streams[CGI_READ_END] == -1) { // Unregister context when there is no reader nor writer
		this->state = DONE;
		Multiplexer::unintroduce_context((uint64_t)this);
	}
}

void Cgi::close_read(void) {
	std::cout << "close_read\n";
	unregister_fd(this->streams[CGI_READ_END]);
	this->streams[CGI_READ_END] = -1;
	if (this->streams[CGI_WRITE_END] == -1) { // Unregister context when there is no reader nor writer
		this->state = DONE;
		Multiplexer::unintroduce_context((uint64_t)this);
	}
}

void Cgi::switch_mode(socket_mode_t mode, int fd) __THROWS_STRERROR
{
	Multiplexer *self;
	self = Multiplexer::get_multiplexer(NULL);
	if (!self) throw "Well, failed to get a Multiplexer class";
	if (!utility::epoll_switch(self->epoll_fd, fd, mode, this))
		throw strerror(errno);
}

void Cgi::append_into_headers_buffer(const char *buffer, ssize_t size)
{
	if (size <= 0)
		return ;
	this->headers_buffer.reserve(this->headers_buffer.size() + size);
	this->headers_buffer.insert(this->headers_buffer.begin(),
			buffer,
			buffer + size);
}

void Cgi::append_into_client_body_buffer(const char *buffer, ssize_t size)
{
	// Appends to the body that will be sent to the client.
	if (size <= 0) {
		this->cgi_done = true;
		return ;
	}
	// Note: if the cgi is chunked. then instead of just sending. send 

	// size | \r\n | body
	// to do:
	//     1) read the size first. then in the next epoll event read the actually chunk.
	//     2) If I need to send.

	//         SEND_SIZE
	//         SEND_CHUNCK (Max is WRITE_CHUNK_SIZE)

	utility::push_into_buffer(this->client_body_buffer, buffer, size);
	this->client_read_bytes += size; 
	if (this->cgi_content_length == -1)
		return ;
	if (this->client_read_bytes >= this->cgi_content_length) {
		this->cgi_done = true;
	}
}

void Cgi::append_into_cgi_body_buffer(const char *buffer,
		ssize_t size,
		ChunkContext *chunk)
{
	if (size <= 0 || this->client_done)
	{
		this->client_done = true;
		return ;
	}
	utility::push_into_buffer(this->cgi_body_buffer, buffer, size);
	this->cgi_read_bytes += size; 
	if (!chunk && this->cgi_read_bytes >= this->client_content_length) this->client_done = true;
	if (chunk && chunk->is_done()) this->client_done = true;
}

void Cgi::send_body_chunk(int conn) __THROWS_STRERROR
{
	ssize_t sent;
	size_t to_send;

	if (this->client_body_buffer.size())
	{
		to_send = std::min((int)WRITE_CHUNK_SIZE, (int)this->client_body_buffer.size());
		sent = ::write(conn, &this->client_body_buffer[0], to_send);
		if (sent > 0) {
			this->client_body_buffer.erase(
				this->client_body_buffer.begin(),
				this->client_body_buffer.begin() + sent);
		}
		else
			this->done();
	}
}

void Cgi::send_headers(int conn) __THROWS_STRERROR
{
	std::string headers = utility::serialize_headers(this->headers, true);
	::send(conn, headers.c_str(), headers.size(), 0);
	this->headers_sent = true;
}

void Cgi::done(void)
{
	if (this->state != DONE && this->state != Idle) {
		this->cgi_done = true;
		this->client_done = true;
		this->close_read();
		this->close_write();
		Multiplexer::unintroduce_context((uint64_t)this);
	}
}

void Cgi::write() __THROWS_STRERROR
{
	ssize_t sent;

	if (this->cgi_body_buffer.size()) {
		sent = Multiplexer::write(this->streams[CGI_WRITE_END],
					&this->cgi_body_buffer[0],
					std::min((unsigned long)WRITE_CHUNK_SIZE, this->cgi_body_buffer.size()));
		if (sent > 0)
		{
			this->cgi_body_buffer
				.erase(this->cgi_body_buffer.begin(), this->cgi_body_buffer.begin() + sent);
		}
		if ((this->client_done && !this->cgi_body_buffer.size()) || sent < 0)
			this->close_write();
		std::cout << "Sent to cgi: " << sent << "\n";
	}
}


std::pair<std::vector<char>::iterator, size_t> Cgi::find_seperator(void)
{
    const char *p1 = CRLF;
    const char *p2 = NLNL;

    if (this->headers_buffer.size() < 4) 
        return std::make_pair(this->headers_buffer.end(), 0);

    std::vector<char>::iterator it = std::search(this->headers_buffer.begin(), 
                                                 this->headers_buffer.end(), 
                                                 p1, p1 + 4);
    if (it != this->headers_buffer.end())
		return std::make_pair(it, 4);
    return std::make_pair(std::search(this->headers_buffer.begin(), 
                       this->headers_buffer.end(), 
                       p2, p2 + 2), 2);
}

bool Cgi::strip_body_if_found(void)
{
	ssize_t leftover;
	std::pair<std::vector<char>::iterator, size_t> seperator =
		this->find_seperator();

	size_t headers_length;
	if (seperator.first == this->headers_buffer.end())
		return (false);
	headers_length = ((seperator.first - this->headers_buffer.begin()));
	leftover = this->headers_buffer.size() - headers_length - seperator.second;
	if (leftover > 0) {
		this->append_into_client_body_buffer((&this->headers_buffer[headers_length] + seperator.second),
					leftover);
	}
	this->headers_buffer
		.resize(headers_length);
	return (true);
}

void Cgi::read() __THROWS_STRERROR
{
	char buffer[READ_CHUNK_SIZE];
	ssize_t read_from_cgi;
	std::vector<char>::iterator  it;

	if (this->state == DONE)
		return ;
	if (this->state == ReadingBody && this->client_body_buffer.size() >= READ_CHUNK_SIZE) // 64KB
		return ;
	std::memset(buffer, 0, sizeof(buffer));
	read_from_cgi = Multiplexer::read(this->streams[CGI_READ_END],
				buffer,
				sizeof(buffer));
	switch (this->state)
	{
		case DONE: {} break;
		case Idle: {
			std::cout << "Wtf bro this should be done in write\n";
			abort();
		} break;        // Idk what is this for tho???
		case ReadingHeaders: {
			this->append_into_headers_buffer(buffer, read_from_cgi);
			if (this->strip_body_if_found())
			{
				try {
					this->parse_headers();
				} catch (const char *e) {
					std::cout << "Failed to parse headers\n";
					this->done();
					throw e;
				}
			}

			if (read_from_cgi <= 0)
				this->close_read();
		} break;
		case ReadingBody: {
			this->append_into_client_body_buffer(buffer, read_from_cgi);	
			if (this->cgi_done) {
				this->close_read();
			}
		} break;
	}
}

bool Cgi::timeout(void) __THROWS_STRERROR
{
	time_t now;

	now = time(NULL);
	if (now < 0)
	{
		this->done();
		throw strerror(errno);
	}
	if ((now - this->last_event_time) >= SCRIPT_TIMEOUT)
	{
		this->done();
		return true;
	}
	return (false);
}

void Cgi::action(uint32_t e) __THROWS_STRERROR
{
	// Note: Check timeout...
	this->last_event_time = time(NULL);
	if (this->timeout())
	{
		std::cout << "Cgi timeout\n";
		this->done();
		return ;
	}
	try {
		if (e & EPOLLIN)
            this->read(); 
        if (e & EPOLLOUT)
            this->write();
		if (e & (EPOLLERR | EPOLLHUP | EPOLLRDHUP))
		{
			if (e & EPOLLERR)   std::cerr << "Crit: EPOLLERR\n";
			if (e & EPOLLHUP)   std::cerr << "Info: EPOLLHUP\n";
			if (e & EPOLLRDHUP) std::cerr << "Info: EPOLLRDHUP\n";
    
			this->done();
			this->gateway_failure();
		}
	} catch (const char *e) {
		this->done();
		throw e;
	}
}

Cgi::Cgi(): ASocketContext()
{
	for (size_t i = 0; environ[i]; ++i) this->env.push_back(environ[i]);

	this->state                  =  Idle;
	this->last_event_time             =  0;
	this->cgi_read_bytes         =  -1; // tracker for body data read from cgi.
	this->cgi_content_length     =  -1; // How much data do u expect from cgi
	this->client_content_length  =  -1; // trac
	this->client_read_bytes      =  -1;
	this->streams[CGI_READ_END ] =  -1;
	this->streams[CGI_WRITE_END] =  -1;
	this->pid                    =  -1;
	this->headers_parsed         = false;
	this->headers_sent           = false;
	this->gateway_failed         = 0;
	this->cgi_done               = false;
	this->client_done            = false;
}

void Cgi::setup(const HttpRequest &request, std::string fn, std::string interpreter_)
{
	this->protocol               =  request.httpVersion;
	this->method                 =  request.method;
	this->query_string           =  request.query_string;
	if (this->method == "POST") {
		this->client_content_length  =  request.content_length;
	}
	this->params                 =  request.params;
	this->filename               =  fn;
	this->executable             =  fn;
	this->interpreter            =  interpreter_;
	this->gateway_interface      =  "CGI/1.1";

	if (request.headers.find("content-type") != request.headers.end())
		this->content_type = request.headers.at("content-type");
	else
		this->content_type = "application/octet-stream";

	this->setup_environment_variables(request.headers);
	if (request.body->size())
	{
		this->append_into_cgi_body_buffer(request.body->data(),
				request.body->size(),
				(ChunkContext*)(request.ischunked * (uint64_t)&request.chunked_context));
		request.body->clear();
	}
}

void Cgi::setup_environment_variables(const std::map<std::string, std::string> &headers) {
	std::stringstream stream;
	std::string key, value;

	stream << this->client_content_length;
	this->env.push_back("REQUEST_METHOD="   + this->method);
	this->env.push_back("QUERY_STRING="     + this->query_string);
	this->env.push_back("CONTENT_LENGTH="   + stream.str());
	this->env.push_back("CONTENT_TYPE="     + this->content_type);
	this->env.push_back("GATEWAY_INTERFACE=" + this->gateway_interface);
	this->env.push_back("SCRIPT_NAME="      + this->filename);
	this->env.push_back("PATH_TRANSLATED="  + this->filename);
	if (headers.find("REMOTE_ADDR") != headers.end())
		this->env.push_back("REMOTE_ADDR="      + headers.at("REMOTE_ADDR"));
	if (headers.find("REMOTE_PORT") != headers.end())
		this->env.push_back("REMOTE_PORT="      + headers.at("REMOTE_PORT"));
	this->env.push_back("SERVER_PROTOCOL="  + this->protocol);

	// Note: send the rest headers as HTTP_*
	for (std::map<std::string, std::string>::const_iterator it = headers.begin(); it != headers.end(); ++it)
	{
		key   = it->first;
		value = it->second;

		if (key == "content-type" || key == "content-length")
			continue ;
		std::transform(key.begin(), key.end(), key.begin(), ::toupper);
		this->env
			.push_back("HTTP_" + key + "=" + value);
	}
}

void Cgi::epoll_register(void) __THROWS_STRERROR
{
	struct epoll_event event;
	Multiplexer *self;

	self = Multiplexer::get_multiplexer(NULL);
	if (!self) throw "Well, failed to get a Multiplexer class";

	if (utility::set_nonblocking(this->streams[CGI_WRITE_END]) == -1) throw strerror(errno);
	if (utility::set_nonblocking(this->streams[CGI_READ_END]) == -1) throw strerror(errno);

    this->state = ReadingHeaders;
    event.events = EPOLLIN | EPOLLRDHUP | EPOLLERR;
    event.data.ptr = this; 
    epoll_ctl(self->epoll_fd,
			EPOLL_CTL_ADD,
			this->streams[CGI_READ_END], &event);

    if (this->method == "POST") {
        event.events = EPOLLOUT;
        event.data.ptr = this;
        epoll_ctl(self->epoll_fd, EPOLL_CTL_ADD, this->streams[CGI_WRITE_END], &event);
    } else {
        close(this->streams[CGI_WRITE_END]);
        this->streams[CGI_WRITE_END] = -1;
    }
	Multiplexer::introduce_new_context((uint64_t)this);
}

bool Cgi::is_executable(void)
{
	if (!utility::check_permissions(this->interpreter)) return (false);
	if (!utility::exists(this->filename)) return (false);
	return (true);
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
	if (!this->is_executable()) throw strerror(errno);
    for (size_t i = 0; i < this->env.size(); ++i)
        envp.push_back((char*)this->env[i].c_str());
    envp.push_back(NULL);
	if (pipe(input) == -1) throw strerror(errno);
	if (pipe(output) == -1) {
		utility::close_fdlist(input);
		throw strerror(errno);
	}
	this->pid = fork();
	if (this->pid == -1)
	{
		utility::close_fdlist(output);
		utility::close_fdlist(input);
		throw strerror(errno);
	}
	if (this->pid == 0)
	{
		if (signal(SIGPIPE, SIG_IGN) == SIG_ERR)         exit(1);
		dup2(input [CGI_READ_END],  STDIN_FILENO);
		dup2(output[CGI_WRITE_END], STDOUT_FILENO);
		int logfd = open(CGI_LOG_FILE, O_WRONLY | O_CREAT | O_APPEND, 0644);
		if (logfd != -1)
		{
			dup2(logfd, STDERR_FILENO);
			close(logfd);
		}
		utility::close_fdlist(output);
		utility::close_fdlist(input);
		execve(this->interpreter.c_str(), args, &envp[0]);
		_exit(127);
	}
	this->last_event_time = 
		time(NULL);
	if (this->last_event_time < 0) {
		utility::close_fdlist(output);
		utility::close_fdlist(input);
		throw strerror(errno);
	}
	this->streams[CGI_READ_END] = output[STDIN_FILENO]; 
	close(input[STDIN_FILENO]);
	this->streams[CGI_WRITE_END] = input[STDOUT_FILENO]; 
	close(output[STDOUT_FILENO]);
	this->epoll_register();
}

void Cgi::gateway_failure(void)
{
	this->gateway_failed = BadGateway;
	std::cout << "gateway_failure\n";
	this->done();
}

void Cgi::parse_headers()    __THROWS_STRERROR
{
	std::vector<std::string> headers;
	std::vector<std::string> pair;

	headers =
		utility::split(this->headers_buffer, "\n");
	if (!utility::isheaders_valid(headers))
	{
		this->gateway_failure();
		return ;
	}
	for (size_t i = 0; i < headers.size(); ++i)
	{
		pair = utility::split_by_two(headers[i], ":");
		if (pair.size() != 2) {
			this->gateway_failure();
			return ;
		}
		std::transform(pair[0].begin(), pair[0].end(), pair[0].begin(), ::tolower);
		this->headers[pair[0]] = pair[1];
	}
	this->state = ReadingBody;
	this->headers_parsed = true;

	if (this->headers.find("content-type") == this->headers.end()) 
		this->headers["content-type"] = TextHtml;
	if (this->headers.find("content-length") == this->headers.end()) 
		return ;
	std::stringstream ss(this->headers.at("content-length"));
	if (!(ss >> this->cgi_content_length)) this->gateway_failure();
}
