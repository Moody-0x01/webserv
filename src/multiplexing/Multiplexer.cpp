#include <Server.hpp>
#include <cstdio>
#include <stdint.h>
#include <cstring>
#include <stdexcept>

int Multiplexer::epoll_fd = 0;
EpollEvent Multiplexer::events[EVENT_MAX];
std::map<int, std::pair<Server, Clients> > Multiplexer::servers;

int set_nonblocking(int sockfd)
{
	errno = 0;
    int flags = fcntl(sockfd, F_GETFL, 0);
    if (flags == -1) {
		std::cerr << "set_nonblocking: " << strerror(errno);
        return -1;
    }
    if (fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) == -1) {
		std::cerr << "set_nonblocking: " << strerror(errno);
        return -1;
    }
    return 0;
}

// Supposed to init all the servers + configure 
void Multiplexer::init(void)
{
	Multiplexer::epoll_fd = epoll_create(1024);
	if (Multiplexer::epoll_fd < 0)
		throw std::runtime_error(std::strerror(errno));
	/*
	 * How it should be used:
	 * 
	 * for (conf in configs) 
	 *		Multiplexer::register_server(conf);
	 * */
	Multiplexer::register_server();
}

void Multiplexer::deinit(void)
{
	close(Multiplexer::epoll_fd);
}

void Multiplexer::unregister_client(int owner, int client)
{
	Multiplexer::servers[owner].second.erase(client);
}

Server *Multiplexer::register_server(void)
{
	int server_fd;
	int opt;
	struct epoll_event event;
    struct sockaddr_in address;
	Server server(server_handler);

    opt = 1;
	server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd < 0) return (NULL);
	server.set_socket(server_fd);
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    std::memset(&address, 0, sizeof(address));
    std::memset(&event, 0, sizeof(event));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8080);

    if (bind(server.get_socket(), (struct sockaddr*)&address, sizeof(address)) < 0) return (NULL);
	if (set_nonblocking(server.get_socket()) == -1) return (NULL);
    if (listen(server.get_socket(), 3) < 0) return (NULL);
	server.disown(); // So it does not close the socket at exit
	Multiplexer::servers[server_fd] = std::make_pair(server, Clients());

    /*  std::cout <<   */
	printf("Created server with fd=%d\n", server_fd);
	event.events = EPOLLIN;
	event.data.ptr = &Multiplexer::servers[server_fd].first;
	int code = epoll_ctl(Multiplexer::epoll_fd, EPOLL_CTL_ADD, server.get_socket(), &event);
	if (code < 0)
	{
		Multiplexer::servers.erase(server_fd); // TODO: closing
		return (NULL); // TODO: raise an exception if this shit fails, do not return an error code
	}

	return ((Server*)event.data.ptr);
}

Client *Multiplexer::register_client(uint32_t e, Server *server)
{
	(void)e;
	struct sockaddr_in addr;
	int conn;
	socklen_t len;
	Client client(client_handler);

	len = sizeof(addr);
	conn = accept(server->get_socket(), (struct sockaddr*)&addr, &len);
	if (conn == -1) return (NULL);
	client.set_owner(server->get_socket());
	client.set_socket(conn);
	if (set_nonblocking(conn) == -1) return (NULL);
	client.disown();
	Multiplexer::servers[server->get_socket()].second[conn] = client;
	return (&Multiplexer::servers[server->get_socket()].second[conn]);
}

int Multiplexer::loop(void)
{
    while (true)
	{
		int ready = epoll_wait(Multiplexer::epoll_fd,
						 Multiplexer::events, EVENT_MAX, 100);
		if (ready < 0)
		{
			std::cerr << "epoll_wait: " << strerror(errno) << "\n";
			return 1;
		}
		for (int index = 0; index < ready; ++index)
		{
			SocketContext *handle = (SocketContext *)(Multiplexer::events[index].data.ptr);
			handle->action(Multiplexer::events[index].events, handle);
			// TODO: Check syscall errors. print using strerror(0)
		}
    }
	return (0);
}
