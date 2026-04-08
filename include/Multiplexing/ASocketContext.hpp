#pragma once
# include <string>
# include <sys/epoll.h>
# include <sys/socket.h>
#include <sys/types.h>
# include <unistd.h>
# include <cassert>
# include <netinet/in.h>
# include <Parser/HTTP/HttpParser.hpp>
# include <Parser/Config/Config.hpp>
# include <HTTP/Response.hpp>
# include <HTTP/Request.hpp>

typedef struct ASocketContext
{
public:
	// TODO: If an assignment operatior is called then it is obvious that the ownership of the socketfd,
	// will be passed to the newly created socket. no need to close it.
	ASocketContext(int sock);
	ASocketContext();
	/*  virtual ASocketContext &ASocketContext::operator=(ASocketContext &Other);  */
	/*  virtual ASocketContext &operator=(const ASocketContext &Other);  */
	virtual ~ASocketContext();


	std::string request_buffer;
	std::string response_buffer;
	void virtual action(uint32_t e) __THROWS_STRERROR = 0;
	void virtual take_ownership(ASocketContext *Other);

	void set_socket(int sockfd);
	int get_socket(void) const;
	void disown(void);
private:
	int  _sockfd;
	bool _owns_fd;
} ASocketContext;


typedef enum cgi_state_e {
	WRITING_BODY,
	READING_RESPONSE
} cgi_state_t;

typedef struct Cgi_: public ASocketContext {
public:
	void action(uint32_t e) __THROWS_STRERROR;
	pid_t       pid;
	int         streams[2];
	cgi_state_t state;
} Cgi_;
