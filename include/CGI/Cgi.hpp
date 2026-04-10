# pragma once
# include <Multiplexing/ASocketContext.hpp>
#include <string>
#include <unistd.h>

typedef enum cgi_state_e {
	Idle,
	WritingBody, // read from write to client
	ReadingBody, // Read then send to child
} cgi_state_t;

extern char **environ;

typedef struct Cgi: public ASocketContext
{
	char buffer[WRITE_CHUNK_SIZE];
	std::string uri;

	std::string method; /*  REQUEST_METHOD: (e.g., GET, POST)  */
	std::map<std::string, std::string> params;
	std::string filename; /* SCRIPT_NAME: script virtual path */
	std::string executable;
	std::string interpreter;
	std::string query_string;  /* QUERY_STRING: (e.g: x=1&y=182) */
	std::string content_type; /*  CONTENT_TYPE: Essential for the script to parse the body (e.g., application/x-www-form-urlencoded).  */
	std::string gateway_interface;  /*  GATEWAY_INTERFACE: Usually "CGI/1.1".  */
	std::string remote_addr; /*  REMOTE_ADDR: The IP of the client.  */
	std::string protocol; /*  SERVER_PROTOCOL: (e.g., "HTTP/1.1").  */

    size_t content_length;      /*  CONTENT_LENGTH: Critical. The script will not read from its stdin if this is missing or 0.  */
    size_t bytes_read_from_cgi; // To know when the script is done
    bool   headers_parsed;      // Flag to track if we are still reading CGI headers

    std::vector<std::string> env; 
    time_t start_time;          // Use this in your loop to kill(pid, SIGKILL) 
                                // if the script takes > 30 seconds.
public:
	Cgi();
	~Cgi() {};
	pid_t       pid;
	int         streams[2];
	cgi_state_t state;

	void setup(std::string fn, std::string interpreter_, std::string qstring, size_t content_length_, std::string content_type_);
	void action(uint32_t e) __THROWS_STRERROR;
	void write()            __THROWS_STRERROR;
	void read()             __THROWS_STRERROR;
} Cgi;

/**/
/*  void Cgi::setup(const HttpRequest &request, std::string fn, std::string interpreter_, size_t content_length_, std::string content_type_)  */
/*  {  */
/*  	std::string content_length_str;  */
/*  	std::stringstream ss;  */
/**/
/*  	this->protocol = request.httpVersion;  */
/*  	this->method = request.method;  */
/*  	this->query_string = request.query_string;  */
/*  	this->params = request.params;  */
/*  	this->filename    = fn;  */
/*  	this->executable  = fn;  */
/*  	this->interpreter = interpreter_;  */
/*  	this->content_length = content_length_;  */
/*  	this->content_type = content_type_;  */
/*  	this->gateway_interface = "CGI/1.1";  */
/**/
/*  	ss << content_length;  */
/*  	content_length_str = ss.str();  */
/*  	this->env.push_back("REQUEST_METHOD="+this->method);  */
/*  	this->env.push_back("QUERY_STRING="+this->query_string);  */
/*  	this->env.push_back("CONTENT_LENGTH="+content_length_str);  */
/*  	this->env.push_back("CONTENT_TYPE="+this->content_type);  */
/*  	this->env.push_back("GATEWAY_INTERFAC="+this->gateway_interface);  */
/*  	this->env.push_back("SCRIPT_NAME="+this->filename);  */
/*  	this->env.push_back("PATH_TRANSLATED="+this->filename);  */
/*  	this->env.push_back("REMOTE_ADDR="+this->);  */
/*  	this->env.push_back("SERVER_PROTOCOL="+this->);  */
/*  	this->env.push_back("SERVER_SOFTWARE="+this->);  */
/*  }  */
