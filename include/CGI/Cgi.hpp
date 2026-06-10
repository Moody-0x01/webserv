# pragma once
# include <Multiplexing/ASocketContext.hpp>
# include <cstring>
# include <string>
# include <unistd.h>
# include <unistd.h>
# include <vector>
# define CRLF "\r\n\r\n"
# define NLNL "\n\n"
# define CGI_LOG_FILE "/tmp/cgi.log"

typedef enum cgi_state_e {
	Idle,
	ReadingHeaders,
	ReadingBody,    // These two states are reading states only, if we are writing the body
					// they dont matter.
	DONE
} cgi_state_t;

extern char **environ;

# define CGI_READ_END  0
# define CGI_WRITE_END 1
# define SCRIPT_TIMEOUT 30
# define MAX_HEADERS_SIZE (4096 * 2)

typedef struct Cgi: public ASocketContext
{
	std::string uri;

	bool gateway_failed; /* http code that will be set when anything bad was found in the parsing phase or reading. */
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

    ssize_t client_content_length;      /*  CONTENT_LENGTH: Critical. The script will not read from its stdin if this is missing or 0.  */
	ssize_t cgi_content_length;

    ssize_t client_read_bytes; // To know when the script is done
    ssize_t cgi_read_bytes; // To know when the script is done
    std::vector<std::string> env; 
	std::map<std::string, std::string> headers;
    time_t last_event_time;          // Use this in your loop to kill(pid, SIGKILL) 
                                // if the script takes > 30 seconds.
public:
	Cgi();
	~Cgi();
	bool        headers_sent;
	bool        headers_parsed;

	bool cgi_done;
	bool client_done;
	pid_t       pid;
	int         streams[2];
	cgi_state_t state;
	std::vector<char> headers_buffer;
	std::vector<char> body_buffer;

	std::vector<char>    cgi_body_buffer; // Cgi writes here and client reads from here.
	std::vector<char> client_body_buffer; // Client writes here then Cgi reads from here.

	void switch_mode(socket_mode_t mode, int fd) __THROWS_STRERROR;
	void setup(const HttpRequest &request, std::string fn, std::string interpreter_);
	void execute(void)          __THROWS_STRERROR;
	void send_headers(int conn) __THROWS_STRERROR;
	void send_body_chunk(int conn) __THROWS_STRERROR;
	void action(uint32_t e)     __THROWS_STRERROR;
	void write()                __THROWS_STRERROR;
	void read()                 __THROWS_STRERROR;
	void parse_headers()        __THROWS_STRERROR;
	void epoll_register(void)   __THROWS_STRERROR;
	bool timeout(void)     __THROWS_STRERROR;
	void free(void);
	void gateway_failure(void);
	void setup_environment_variables(const std::map<std::string, std::string> &headers);
	void append_into_client_body_buffer(const char *buffer, ssize_t size);
	void append_into_cgi_body_buffer(const char *buffer, ssize_t size);
	void append_into_headers_buffer(const char *buffer, ssize_t size);
	void append_into_cgi_body_buffer(const char *buffer,
			ssize_t size,
			ChunkContext *chunk);
	bool validate_headers();
	bool is_executable(void);
	std::pair<std::vector<char>::iterator, size_t> find_seperator(void);
	bool strip_body_if_found(void);
	void close_write(void);
	void close_read(void);
	void done(void);
	bool did_fail() const;
} Cgi;
