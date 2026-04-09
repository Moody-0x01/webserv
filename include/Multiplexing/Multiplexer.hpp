#pragma once
# include "ServerSock.hpp"
# include <cstddef>
# include <Multiplexing/ClientSock.hpp>
# include <map>
# include <sys/types.h>
# include <sys/socket.h>
# include <sys/types.h>
# include <sys/wait.h>
# include <cerrno>
# include <csignal>
# include <cstdio>
# include <netdb.h>
# include <stdint.h>
# include <string>
# include <unistd.h>
# include <signal.h>
# include <iostream>
# include <errno.h>
# include <cstddef>
# include <cstring>
# include <stdexcept>
# include <fcntl.h>
# include <utility>

# define EVENT_MAX 4096
# define IGNORED 1
# define __THROWS_STRERROR throw(const char *)

typedef std::map<int, Client> Clients;
typedef struct epoll_event EpollEvent;

// Note: instead of having this class as a static class that has bunch of static methods, I think it is better to just make it a singelton.

class Multiplexer {
private:
	Multiplexer() __THROWS_STRERROR;

public:
	EpollEvent events[EVENT_MAX];
	std::map<int, ServerConfig> confs;
	std::map<int, std::pair<Server, Clients> > servers;
	const int epoll_fd;
	int signal_io[2];
	~Multiplexer();

	void init(std::vector<ServerConfig> &confs) throw(std::runtime_error, const char *);
	int loop(void);
	void deinit(void);
	void init_signals(void) __THROWS_STRERROR;
	Server *get_owner(int fd) __THROWS_STRERROR;

	static const ServerConfig &get_conf(int fd);
	static void     register_server(ServerConfig &conf) __THROWS_STRERROR;
	static Client   *register_client(uint32_t e, Server *server) __THROWS_STRERROR;
	static void unregister_client(int owner, int client) __THROWS_STRERROR;
	static ssize_t  read(int fd, void *buf, size_t size) __THROWS_STRERROR;
	static ssize_t  write(int fd, const void *buf, size_t size) __THROWS_STRERROR;
	static Multiplexer *create_multiplexer(std::vector<ServerConfig> &confs);
	static Multiplexer *get_multiplexer(std::vector<ServerConfig> *confs) throw(std::runtime_error, const char *);
};


std::string get_signal_name(int sig);
int set_nonblocking(int sockfd);
void signal_handler(int sig);
