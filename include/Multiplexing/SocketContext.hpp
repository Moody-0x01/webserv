#pragma once
# include <string>
# include <sys/epoll.h>
# include <sys/socket.h>
# include <unistd.h>
# include <cassert>
# include <netinet/in.h>
# include <Parser/HTTP/HttpParser.hpp>
# include <Parser/Config/Config.hpp>

typedef struct SocketContext SocketContext;
typedef SocketContext Client;
typedef SocketContext Server;
typedef void (*SocketHandler)(uint32_t , SocketContext *);

typedef struct SocketContext
{
public:
	// TODO: If an assignment operatior is called then it is obvious that the ownership of the socketfd,
	// will be passed to the newly created socket. no need to close it.
	SocketContext(SocketHandler a);
	SocketContext(SocketHandler a, int sock);
	SocketContext &operator=(const SocketContext &Other);
	SocketContext();
	~SocketContext();

	void set_socket(int sockfd);
	int get_socket(void) const;
	void set_owner(int owner);
	int get_owner(void) const;
	void disown(void);

	std::string request_buffer;
	std::string response_buffer;
	SocketHandler action;

	// TODO: add and implement Request and Response class
	// Request request;
    // Response response;
	// For the client instances:
		HttpParser parserInstance;
	// For the server instances:
		ServerConfig conf;
	void free();
	HttpParser &getParser();
private:
	int  _sockfd;
	int  _owner;
	bool _owns_fd;

} SocketContext;

// NOTE: The SocketContext is an abstraction that wrap implementations of both the server and the client.
// So whatever a registered client or a server does in `Epoll` is determined using the action method.
// and whatever data they have must be decided by those two action functions.
