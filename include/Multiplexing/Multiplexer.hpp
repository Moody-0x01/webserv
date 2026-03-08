#pragma once

# include <map>
# include <Multiplexing/SocketHandlers.hpp>

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
	static Server *register_server(ServerConfig &conf);
	// TODO: Implement, registration of a new client
	static Client *register_client(uint32_t e, Server *server);
	static void   unregister_client(int owner, int client);
	// TODO: When the Multiplexer is constructed it should init all the current servers that have been
	// parsed by the configuration.
	static void init(std::vector<ServerConfig> &confs); // Should take configuration.
	static std::string resolve_host(const std::string &host);
	static int loop(void);
	static void deinit(void);
	// TODO: When the Multiplexer is deconstructed it shuld actually make sure that every Server was dealloated successfully
	// it should also remove the allocated epoll instance. then exit cleanly


};
