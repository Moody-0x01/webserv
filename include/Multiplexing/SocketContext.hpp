#pragma once
# include <iostream>
# include <string>
# include <sys/epoll.h>
# include <sys/socket.h>
# include <unistd.h>
# include <cassert>
# include <netinet/in.h>
# include <vector>
# include <map>

typedef struct SocketContext SocketContext;
typedef SocketContext Client;
typedef SocketContext Server;
typedef void (*SocketHandler)(uint32_t , SocketContext *);

typedef struct SocketContext
{
public:
	SocketContext(SocketHandler a);
	SocketContext(SocketHandler a, int sock);
	SocketContext();
	~SocketContext();

	void set_socket(int sockfd) { _sockfd = sockfd; }
	int get_socket(void) const { return _sockfd; }

	void set_owner(int owner) { _owner = owner; }
	int get_owner(void) const { return _owner; }

	std::string request_buffer;
	std::string response_buffer;
	SocketHandler action;

private:
	int  _sockfd;
	int  _owner;
	bool _owns_fd;
} SocketContext;
