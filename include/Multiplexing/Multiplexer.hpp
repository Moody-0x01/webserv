#pragma once
# include <cstddef>
# include <Multiplexing/ClientSock.hpp>
# include <map>
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
	static int signal_io[2];
	static EpollEvent events[EVENT_MAX];
	static std::map<int, ServerConfig> confs;
	static std::map<int, std::pair<Server, Clients> > servers;
	static void  register_server(ServerConfig &conf) __THROWS_STRERROR;
	static Client   *register_client(uint32_t e, Server *server) __THROWS_STRERROR;
	static void   unregister_client(int owner, int client);
	static void init(std::vector<ServerConfig> &confs) throw(std::runtime_error, const char *);
	static std::string resolve_host(const std::string &host);
	static int loop(void);
	static void deinit(void);
	static ssize_t read(int fd, void *buf, size_t size) __THROWS_STRERROR;
	static void init_signals(void) __THROWS_STRERROR;
	static ssize_t write(int fd, const void *buf, size_t size) __THROWS_STRERROR;
};

std::string get_signal_name(int sig);
void signal_handler(int sig);
