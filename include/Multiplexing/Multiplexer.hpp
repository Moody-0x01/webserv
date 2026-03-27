#pragma once

#include <cstddef>
# include <map>
# include <Multiplexing/SocketHandlers.hpp>
#include <sys/types.h>

# define EVENT_MAX 4096
# define IGNORED 1
# define __THROWS_STRERROR throw(const char *)
int set_nonblocking(int sockfd);

typedef std::map<int, Client> Clients;
typedef struct epoll_event EpollEvent;

class Multiplexer {
public:
	static int epoll_fd;
	static EpollEvent events[EVENT_MAX];
	static std::map<int, std::pair<Server, Clients> > servers;
	static std::map<int, std::string> status_lines;
	// TODO: Implement, registration of a new server
	static void  register_server(ServerConfig &conf) __THROWS_STRERROR;
	static Client   *register_client(uint32_t e, Server *server) __THROWS_STRERROR;
	// TODO: Implement, registration of a new client
	static void   unregister_client(int owner, int client);
	// TODO: When the Multiplexer is constructed it should init all the current servers that have been
	// parsed by the configuration.
	static void init(std::vector<ServerConfig> &confs) throw(std::runtime_error); // Should take configuration.
	static std::string resolve_host(const std::string &host);
	static int loop(void);
	static void deinit(void);
	/*  static ssize_t io_dispatcher(IoFunc f, int fd, char *buff, size_t size);  */
	static ssize_t read(int fd, void *buf, size_t size) __THROWS_STRERROR;
	static ssize_t write(int fd, const void *buf, size_t size) __THROWS_STRERROR;
	// TODO: When the Multiplexer is deconstructed it shuld actually make sure that every Server was dealloated successfully
	// it should also remove the allocated epoll instance. then exit cleanly
};
