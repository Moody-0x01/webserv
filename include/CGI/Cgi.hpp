# pragma once
# include <Multiplexing/ASocketContext.hpp>
# include <string>
# include <unistd.h>
# include <unistd.h>

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

	void setup(const HttpRequest &request, std::string fn, std::string interpreter_);
	void execute();
	void action(uint32_t e) __THROWS_STRERROR;
	void write()            __THROWS_STRERROR;
	void read()             __THROWS_STRERROR;
} Cgi;
