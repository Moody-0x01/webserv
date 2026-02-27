#pragma once

# include <Multiplexing/SocketContext.hpp>

# define EVENT_MAX 4096
int set_nonblocking(int sockfd);

typedef std::map<int, Client> Clients;
typedef struct epoll_event EpollEvent;

class Multiplexer {
public:
	static int epoll_fd;
	static EpollEvent events[EVENT_MAX];
	static std::map<int, std::pair<Server, Clients> > servers;
	// TODO: Implement, registration of a new server
	static Server *register_server(void);
	// TODO: Implement, registration of a new client
	static Client *register_client(int server_fd, int client_fd);
	static int loop(void);
	// TODO: When the Multiplexer is constructed it should init all the current servers that have been
	// parsed by the configuration.
	static void init(void); // Should take configuration.
	static void deinit(void);
	// TODO: When the Multiplexer is deconstructed it shuld actually make sure that every Server was dealloated successfully
	// it should also remove the allocated epoll instance. then exit cleanly


};
