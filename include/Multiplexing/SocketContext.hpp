#pragma once
# include <string>
# include <sys/epoll.h>
# include <sys/socket.h>
# include <unistd.h>
# include <cassert>
# include <netinet/in.h>

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

private:
	int  _sockfd;
	int  _owner;
	bool _owns_fd;
} SocketContext;
