#pragma once

# include <Multiplexing/SocketContext.hpp>

# define EVENT_MAX 4096
int set_nonblocking(int sockfd);

typedef std::map<int, Client> Clients;
typedef struct epoll_event EpollEvent;

template <size_t Size = EVENT_MAX>
class Multiplexer {
public:
	static int epoll_fd;
	static EpollEvent events[Size];
	static std::map<Server, Clients> servers;

	int loop(void);
	// TODO: When the Multiplexer is constructed it should init all the current servers that have been
	// parsed by the configuration.
	Multiplexer(); // Should take configuration.

	// TODO: When the Multiplexer is deconstructed it shuld actually make sure that every Server was dealloated successfully
	// it should also remove the allocated epoll instance. then exit cleanly
	~Multiplexer();

};
