#include <Server.hpp>
#include <sstream>

void Cgi::write() __THROWS_STRERROR
{
}

void Cgi::read() __THROWS_STRERROR
{
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
	{
		// TODO: Read from the process and hand to the client somewhere else.
		this->read();
	}
    if ((e & EPOLLOUT) || (e & EPOLLRDHUP))
	{
		// TODO: Read the body from the client then hand it to the script.
		this->write();
	}
	} catch (const char *e) {
		throw e;
	}
}

Cgi::Cgi()
{
	for (size_t i = 0; environ[i]; ++i) this->env.push_back(environ[i]);
	this->start_time          = 0;
	this->content_length      = 0;
	this->bytes_read_from_cgi = 0;
}

void Cgi::setup(const HttpRequest &request, std::string fn, std::string interpreter_)
{
	std::stringstream stream;

	this->protocol = request.httpVersion;
	this->method = request.method;
	this->query_string = request.query_string;
	this->content_length = request.content_length;
	this->params = request.params;

	this->filename    = fn;
	this->executable  = fn;
	this->interpreter = interpreter_;
	this->gateway_interface = "CGI/1.1";
	if (request.headers.find("content-type") != request.headers.end())
		this->content_type = request.headers.at("content-type");
	else
		this->content_type = "application/octet-stream";
	stream << this->content_length;
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

void Cgi::execute(void) __THROWS_STRERROR
{
	// TODO: fork... dup...
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
		dup2(input[STDIN_FILENO], STDIN_FILENO);
		dup2(output[STDOUT_FILENO], STDOUT_FILENO);
		close_fdlist(output);
		close_fdlist(input);
		execve(this->interpreter.c_str(), args, &envp[0]);
		perror("execve");
		_exit(1);
	}
	this->streams[STDOUT_FILENO] = input[STDOUT_FILENO];
	close(input[STDIN_FILENO]);
	this->streams[STDIN_FILENO] = output[STDIN_FILENO];
	close(output[STDOUT_FILENO]);
}
