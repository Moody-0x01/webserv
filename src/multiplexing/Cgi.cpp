#include <Server.hpp>
#include <cerrno>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <iostream>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

Cgi::~Cgi() {
	std::cout << "brother!! Cgi is done!!\n";
	this->done();
}

void Cgi::append_into_headers_buffer(const char *buffer, ssize_t size)
{
	this->headers_buffer.reserve(this->headers_buffer.size() + size);
	this->headers_buffer.insert(this->headers_buffer.begin(),
			buffer,
			buffer + size);
}

void Cgi::append_into_body_buffer(const char *buffer, ssize_t size)
{
	this->body_buffer.insert(this->body_buffer.end(),
			buffer,
			buffer + size);
}

void Cgi::send_body_chunk(int conn) __THROWS_STRERROR
{
	ssize_t sent;
	size_t to_send;

	if (this->body_buffer.size())
	{
		to_send = std::min((int)WRITE_CHUNK_SIZE, (int)this->body_buffer.size());
		sent = ::write(conn, &this->body_buffer[0], to_send);
		if (sent > 0) {
			this->body_buffer.erase(
				this->body_buffer.begin(),
				this->body_buffer.begin() + sent);
		} else this->done();
	}
}

void Cgi::send_headers(int conn) __THROWS_STRERROR
{
	std::string headers = serialize_headers(this->headers, true);
	::send(conn, headers.c_str(), headers.size(), 0);
	this->headers_sent = true;
}

void Cgi::done(void)
{
	if (this->state != DONE && this->state != Idle) {
		this->state = DONE;
		std::cout << "We done reading!\n";
		std::cout << "Now close pipes: \n";

		unregister_fd(this->streams[CGI_READ_END]);
		unregister_fd(this->streams[CGI_WRITE_END]);
		this->streams[CGI_READ_END] = -1;
		this->streams[CGI_WRITE_END] = -1;
	}
}

void Cgi::write() __THROWS_STRERROR
{
	ssize_t sent;
	if (this->state == WritingBody)
	{
		sent = Multiplexer::write(this->streams[CGI_WRITE_END],
					&this->body_buffer[0],
					this->body_buffer.size());
		if (sent > 0) {
			this->body_buffer.erase(this->body_buffer.begin(), this->body_buffer.begin() + sent);
			this->client_read_bytes += sent;
		}
		if (this->client_content_length >= this->client_read_bytes) {
			this->state = ReadingHeaders;
		}
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
	std::pair<std::vector<char>::iterator, size_t> seperator =
		this->find_seperator();

	size_t headers_length;
	if (seperator.first == this->headers_buffer.end())
		return (false);
	headers_length = ((seperator.first - this->headers_buffer.begin()) + 1);
	this->append_into_body_buffer((&this->headers_buffer[headers_length] + seperator.second),
				this->headers_buffer.size() - headers_length - seperator.second);
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
	if (this->state == ReadingBody && this->body_buffer.size() >= 2048) // 64KB
		return ;
	std::memset(buffer, 0, sizeof(buffer));
	read_from_cgi = Multiplexer::read(this->streams[CGI_READ_END],
				buffer,
				sizeof(buffer));
	if (read_from_cgi == 0 || read_from_cgi == -1) {
		this->done();
		return ;
	}
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
			this->append_into_headers_buffer(buffer, read_from_cgi);
			if (this->strip_body_if_found())
			{
				this->parse_headers();
				this->state = ReadingBody;
			}
		} break;
		case ReadingBody: {
			this->append_into_body_buffer(buffer, read_from_cgi);
			this->cgi_read_bytes += read_from_cgi; 
			if ((this->cgi_content_length != -1)
				&& (this->cgi_read_bytes >= this->cgi_content_length))
				this->done();
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
	if ((now - this->start_time) >= SCRIPT_TIMEOUT)
	{
		this->done();
		return true;
	}
	return (false);
}

void Cgi::action(uint32_t e) __THROWS_STRERROR
{
	// Note: Check timeout...
	try {
		if (e & EPOLLERR) {
            this->done();
            return;
        }
		if (e & EPOLLIN)
            this->read(); 
        if (e & EPOLLOUT)
            this->write();
        if ((e & EPOLLHUP) || (e & EPOLLRDHUP))
            this->done();
	} catch (const char *e) {
		this->done();
		throw e;
	}
}

Cgi::Cgi(): ASocketContext()
{
	for (size_t i = 0; environ[i]; ++i) this->env.push_back(environ[i]);
	this->start_time             =  0;
	this->cgi_read_bytes         =  -1; // tracker for body data read from cgi.
	this->cgi_content_length     =  -1; // How much data do u expect from cgi
	this->client_content_length  =  -1; // trac
	this->client_read_bytes      =  -1;
	this->state                  =  Idle;
	this->streams[CGI_READ_END ] =  -1;
	this->streams[CGI_WRITE_END] =  -1;
	this->pid                    =  -1;
	this->headers_parsed         = false;
	this->headers_sent           = false;
}

void Cgi::setup(const HttpRequest &request, std::string fn, std::string interpreter_)
{
	std::stringstream stream;

	this->protocol               =  request.httpVersion;
	this->method                 =  request.method;
	this->query_string           =  request.query_string;
	if (this->method == "POST")
		this->client_content_length  =  request.content_length;
	this->params                 =  request.params;
	this->filename               =  fn;
	this->executable             =  fn;
	this->interpreter            =  interpreter_;
	this->gateway_interface      =  "CGI/1.1";

	if (request.headers.find("Content-Type") != request.headers.end())
		this->content_type = request.headers.at("Content-Type");
	else
		this->content_type = "application/octet-stream";
	stream << this->client_content_length;
	this->env.push_back("REQUEST_METHOD="   + this->method);
	this->env.push_back("QUERY_STRING="     + this->query_string);
	this->env.push_back("CONTENT_LENGTH="   + stream.str());
	this->env.push_back("CONTENT_TYPE="     + this->content_type);
	this->env.push_back("GATEWAY_INTERFACE=" + this->gateway_interface);
	this->env.push_back("SCRIPT_NAME="      + this->filename);
	this->env.push_back("PATH_TRANSLATED="  + this->filename);
	this->env.push_back("REMOTE_ADDR="      + request.headers.at("REMOTE_ADDR"));  // TODO: Get the ip of the client and forward it to the cgi.
	this->env.push_back("SERVER_PROTOCOL="  + this->protocol);
}

void Cgi::epoll_register(void) __THROWS_STRERROR
{
	struct epoll_event event;
	Multiplexer *self;

	self = Multiplexer::get_multiplexer(NULL);
	if (!self) throw "Well, failed to get a Multiplexer class";

	if (set_nonblocking(this->streams[CGI_WRITE_END]) == -1) throw strerror(errno);
	if (set_nonblocking(this->streams[CGI_READ_END]) == -1) throw strerror(errno);

    event.events = EPOLLIN | EPOLLRDHUP | EPOLLERR;
    event.data.ptr = this; 
    epoll_ctl(self->epoll_fd,
			EPOLL_CTL_ADD,
			this->streams[CGI_READ_END], &event);

    if (this->method == "POST") {
        event.events = EPOLLOUT | EPOLLERR;
        event.data.ptr = this;
        epoll_ctl(self->epoll_fd, EPOLL_CTL_ADD, this->streams[CGI_WRITE_END], &event);
        this->state = WritingBody; 
    } else {
        this->state = ReadingHeaders;
        close(this->streams[CGI_WRITE_END]);
        this->streams[CGI_WRITE_END] = -1;
    }
}

bool Cgi::is_executable(void)
{
	if (!check_permissions(this->interpreter)) return (false);
	if (!exists(this->filename)) return (false);
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
		if (signal(SIGPIPE, SIG_IGN) == SIG_ERR)         exit(1);

		dup2(input [CGI_READ_END],  STDIN_FILENO);
		dup2(output[CGI_WRITE_END], STDOUT_FILENO);

		int logfd = open("/tmp/cgi.log", O_WRONLY | O_CREAT | O_APPEND, 0644);
		if (logfd != -1) {
			dup2(logfd, STDERR_FILENO);
			close(logfd);
		}
		close_fdlist(output);
		close_fdlist(input);
		execve(this->interpreter.c_str(), args, &envp[0]);
		_exit(127);
	}
	this->start_time = 
		time(NULL);
	if (this->start_time < 0) {
		close_fdlist(output);
		close_fdlist(input);
		throw strerror(errno);
	}
	this->streams[CGI_READ_END] = output[STDIN_FILENO]; 
	close(input[STDIN_FILENO]);
	this->streams[CGI_WRITE_END] = input[STDOUT_FILENO]; 
	close(output[STDOUT_FILENO]);
	this->epoll_register();
}

void Cgi::parse_headers()    __THROWS_STRERROR
{
	std::vector<std::string> headers;
	std::vector<std::string> pair;

	this->headers_parsed = true;
	headers =
		split(this->headers_buffer, "\n");
	for (size_t i = 0; i < headers.size(); ++i)
	{
		pair = split(headers[i], " :");
		if (pair[0] == "Status" && pair.size() == 3) {
			//            Status:    xxx             OK?
			this->headers[pair[0]] = pair[1] + " " + pair[2];
		} else if (pair.size() != 2)
		{
			std::cout << "H: " << headers[i] << "\n";
			throw "Invalid Header";
		}
		else
			this->headers[pair[0]] = pair[1];
	}
	if (this->headers.find("Content-Length") == this->headers.end()) 
	{
		/*  this->headers["Transfer-Encoding"] = "chunked";  */
		return ;
	}
	std::stringstream ss(this->headers.at("Content-Length"));
	ss << this->cgi_content_length;
}
