# include <Server.hpp>
// # include <algorithm>
# include <cerrno>
# include <cstdio>
# include <iostream>
# include <string>
# include <sys/epoll.h>
# include <sys/socket.h>
# include <unistd.h>
# include <cassert>
#include <vector>

typedef struct SocketContext SocketContext;
typedef SocketContext Client;
typedef SocketContext Server;
typedef void (*SocketHandler)(uint32_t , SocketContext *);

typedef struct SocketContext
{
public:
	static int epoll_fd;
	SocketContext(SocketHandler a);
	SocketContext(SocketHandler a, int sock);
	SocketContext();
	~SocketContext();

	void set_socket(int sockfd) { _sockfd = sockfd; }
	int get_socket(void) const { return _sockfd; }
	std::string request_buffer;
	std::string response_buffer;
	SocketHandler action;

private:
	int _sockfd;
} SocketContext;

int SocketContext::epoll_fd = 0;

SocketContext::SocketContext(): request_buffer(""), response_buffer(""), action(NULL), _sockfd(-1)
{
}

SocketContext::SocketContext(SocketHandler a, int sock): request_buffer(""), response_buffer(""), action(a), _sockfd(sock)
{
}

SocketContext::SocketContext(SocketHandler a): request_buffer(""), response_buffer(""), action(a), _sockfd(-1)
{
}

SocketContext::~SocketContext()
{
	if (_sockfd != -1) close(_sockfd);
	std::cout << "Closed -> " << _sockfd << "\n";
	_sockfd = -1;
}

const char* http10_ok_header =
    "HTTP/1.0 200 OK\r\n"
    "Content-Type: text/plain\r\n\r\n";

std::map<int, Client&> clients;
# define EVENT_MAX 4096

void client_handler(uint32_t e, Client *Self)
{
	char buff[4096];
	int client;
	ssize_t count;

	client = Self->get_socket();
	if (e & EPOLLOUT) {
		std::cout << "Writing to conn: " << client << "\n";;
		count = read(client, buff, sizeof(buff));
		if (count > 0) clients[client].request_buffer += buff;
		clients[client].response_buffer = http10_ok_header + clients[client].request_buffer;
		count = write(client, clients[client].response_buffer.c_str(), clients[client].response_buffer.size());
		if (count == -1) std::cerr << "write: " << strerror(errno) << "\n";
		close(client);
		clients.erase(client);
		epoll_ctl(SocketContext::epoll_fd, EPOLL_CTL_DEL, client, NULL);
	} else if (e & EPOLLIN) {
		std::cout << "Reading from conn: " << client << "\n";;
		count = read(client, buff, sizeof(buff));
		std::cout << "Read: " << count << "\n";
		if (count > 0) {
			clients[client].request_buffer += buff;
		} else if (count <= 0) {
			if (count < 0)
				std::cerr << "read: " << strerror(errno) << "\n";
			clients.erase(client);
			return ;
		}
		if (clients[client].request_buffer.find("\r\n\r\n") != std::string::npos)
		{
			struct epoll_event cev;
			cev.events = EPOLLOUT;
			cev.data.ptr = ((void *)&clients[client]);
			if (epoll_ctl(SocketContext::epoll_fd, EPOLL_CTL_MOD, client, &cev) ==
			-1) {
				std::cerr << "epoll_ctl: " << strerror(errno) << "\n";
				clients.erase(client);
			}
			std::cout << "Client " << client << " is done sending http/1.0\n";
		}
	}
}

void server_handler(uint32_t e, Server *Self)
{
	(void)e;
	struct sockaddr_in addr;
	socklen_t len = sizeof(addr);
	int conn = accept(Self->get_socket(), (struct sockaddr*)&addr, &len);
	if (conn == -1)
	{
		std::cerr << "accept: " << strerror(errno) << "\n";
		return ;
	}
	// TODO: This makes an unwanted temperary variable, of T: Client
	// Maybe that should not be the case, and any memory that is not needed should
	// not be allocated.
	clients[conn] = Client(client_handler);
	clients[conn].set_socket(conn);
	std::cout << "All set\n";
	struct epoll_event cev;
	cev.events = EPOLLIN;
	cev.data.ptr = ((void*)&clients[conn]);
	set_nonblocking(conn);
	if (epoll_ctl(SocketContext::epoll_fd, EPOLL_CTL_ADD, conn, &cev) == -1)
	{
		std::cerr << "epoll_ctl: " << strerror(errno) << "\n";
		clients.erase(conn);
		return ;
	}
	std::cout << "Accepted a conn: " << conn << "\n";
}

int main() {	 
	struct epoll_event events[EVENT_MAX];
	struct epoll_event event;
    struct sockaddr_in address;
	Server server(server_handler, socket(AF_INET, SOCK_STREAM, 0));
    int opt = 1;

    if (server.get_socket() < 0) {
        std::cerr << "Socket creation failed\n";
        return 1;
    }
    setsockopt(server.get_socket(), SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    std::memset(&address, 0, sizeof(address));
    std::memset(&event, 0, sizeof(event));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8080);

    if (bind(server.get_socket(), (struct sockaddr*)&address, sizeof(address)) < 0) {
        std::cerr << "Bind failed\n";
        close(server.get_socket());
        return 1;
    }
	if (set_nonblocking(server.get_socket()) == -1) return 1;

    if (listen(server.get_socket(), 3) < 0) {
        std::cerr << "Listen failed\n";
        close(server.get_socket());
        return 1;
    }

	SocketContext::epoll_fd = epoll_create(1024);
	if (SocketContext::epoll_fd < 0)
	{
        std::cerr << "Epoll failed\n";
        close(server.get_socket());
        return 1;
	}
    std::cout << "HTTP/1.0 Server listening on localhost:8080\n";
	event.events = EPOLLIN;
	event.data.ptr = (&server);
	int code = epoll_ctl(SocketContext::epoll_fd, EPOLL_CTL_ADD, server.get_socket(), &event);
	if (code < 0)
	{
        std::cerr << "Epoll ctl failed\n";
        return 1;
	}
    while (true)
	{
		int ready = epoll_wait(SocketContext::epoll_fd, events, EVENT_MAX, 100);
		if (ready < 0)
		{
			std::cerr << "epoll_wait: " << strerror(errno) << "\n";
			return 1;
		}
		for (int index = 0; index < ready; ++index)
		{
			Client *handle = (Client *)(events[index].data.ptr);
			std::cout << "Next: " << handle->get_socket() << "\n";
			handle->action(events[index].events, handle);
		}
    }
    return 0;
}
