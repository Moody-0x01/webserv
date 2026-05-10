#pragma once

#include <stdint.h>
#include <stdint.h>
# include <unistd.h>
# include "ServerSock.hpp"
#include "SignalHandler.hpp"
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
# include <signal.h>
# include <iostream>
# include <errno.h>
# include <cstddef>
# include <cstring>
# include <fcntl.h>
# include <utility>
# include <set>

# define EVENT_MAX 4096
# define IGNORED 1
# define __THROWS_STRERROR throw(const char *)

typedef struct epoll_event EpollEvent;
typedef std::map<int, Client> Clients;

class Multiplexer {
private:
	Multiplexer() __THROWS_STRERROR;
	std::set<uintptr_t> _valid_context;
	std::set<Client*>   connected_clients;
	bool aborted;

public:
	EpollEvent events[EVENT_MAX];
	
	std::map<int, ServerConfig> confs;
	std::map<int, std::pair<Server, Clients> > servers;

	const int epoll_fd;
	SignalHandler signal_handler;

	~Multiplexer();
	void   init(std::vector<ServerConfig> &confs) throw(std::runtime_error, const char *);
	int    run(void);
	void   deinit(void);
	void   init_signals(void) __THROWS_STRERROR;
	Server *get_owner(int fd) __THROWS_STRERROR;
	void   execute_epoll_event(int epoll_index);
	void   abort(void);

	static void         kill(void);
	static void			introduce_new_context(uintptr_t context, bool is_client = false);
	static void			unintroduce_context(uint64_t context);
	static bool			iscontext_valid(uint64_t context);
	static const ServerConfig &get_conf(int fd);
	static void			register_server(ServerConfig &conf) __THROWS_STRERROR;
	static Client		*register_client(uint32_t e, Server *server) __THROWS_STRERROR;
	static void			unregister_client(int owner, int client) __THROWS_STRERROR;
	static ssize_t		read(int fd, void *buf, size_t size) __THROWS_STRERROR;
	static ssize_t		write(int fd, const void *buf, size_t size) __THROWS_STRERROR;
	static Multiplexer *create_multiplexer(std::vector<ServerConfig> &confs);
	static Multiplexer *get_multiplexer(std::vector<ServerConfig> *confs) throw(std::runtime_error, const char *);
	void check_connections_timeout(void);
};

void unregister_fd(int fd);
