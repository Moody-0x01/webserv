#include <Server.hpp>
#include <unistd.h>

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
	std::string content_length_str;
	std::stringstream ss;

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


	ss << content_length;
	content_length_str = ss.str();
	this->env.push_back("REQUEST_METHOD="+this->method);
	this->env.push_back("QUERY_STRING="+this->query_string);
	this->env.push_back("CONTENT_LENGTH="+content_length_str);
	this->env.push_back("CONTENT_TYPE="+this->content_type);
	this->env.push_back("GATEWAY_INTERFAC="+this->gateway_interface);
	this->env.push_back("SCRIPT_NAME="+this->filename);
	this->env.push_back("PATH_TRANSLATED="+this->filename);
	/*  this->env.push_back("REMOTE_ADDR="+this->);  */ // TODO: Get the ip of the client and forward it to the cgi.
	this->env.push_back("SERVER_PROTOCOL="+this->protocol);
}

void Cgi::execute(void)
{
	char *args[2] = {(char*)this->filename.data(), NULL};
	std::vector<char*> envp;
    for (size_t i = 0; i < this->env.size(); ++i)
        envp.push_back((char*)this->env[i].c_str());
    envp.push_back(NULL);
	execve(this->interpreter.data(), args, &envp[0]);
	
}
