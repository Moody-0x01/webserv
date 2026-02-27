#include <Server.hpp>
#include <cstddef>
#include <cstring>
#include <stdexcept>
#include <string.h>
#include <fcntl.h>
#include <utility>

int set_nonblocking(int sockfd) {
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


void client_handler(uint32_t e, Client *Self)
{
	char buff[4096];
	int client;
	ssize_t count;

	client = Self->get_socket();
	if (e & EPOLLOUT) {
		std::cout << "Writing to conn: " << client << "\n";;
		count = read(client, buff, sizeof(buff));
		// if (count > 0) clients[client].request_buffer += buff;
		// clients[client].response_buffer = http10_ok_header + clients[client].request_buffer;
		// count = write(client, clients[client].response_buffer.c_str(), clients[client].response_buffer.size());
		// if (count == -1) std::cerr << "write: " << strerror(errno) << "\n";
		// close(client);
		// clients.erase(client);
		epoll_ctl(SocketContext::epoll_fd, EPOLL_CTL_DEL, client, NULL);
	} else if (e & EPOLLIN) {
		std::cout << "Reading from conn: " << client << "\n";;
		count = read(client, buff, sizeof(buff));
		std::cout << "Read: " << count << "\n";
		if (count > 0) {
			// clients[client].request_buffer += buff;
		} else if (count <= 0) {
			if (count < 0)
				std::cerr << "read: " << strerror(errno) << "\n";
			// clients.erase(client);
			return ;
		}
		// if (clients[client].request_buffer.find("\r\n\r\n") != std::string::npos)
		{
			struct epoll_event cev;
			cev.events = EPOLLOUT;
			// cev.data.ptr = ((void *)&clients[client]);
			if (epoll_ctl(SocketContext::epoll_fd, EPOLL_CTL_MOD, client, &cev) ==
			-1) {
				std::cerr << "epoll_ctl: " << strerror(errno) << "\n";
				// clients.erase(client);
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
	// Multiplexer::servers[Self->get_socket()]
	// clients[conn] = Client(client_handler);
	// clients[conn].set_socket(conn);
	// clients[conn].set_owner(Self->get_socket());
	std::cout << "All set\n";
	struct epoll_event cev;
	cev.events = EPOLLIN;
	// cev.data.ptr = ((void*)&clients[conn]);
	set_nonblocking(conn);
	if (epoll_ctl(SocketContext::epoll_fd, EPOLL_CTL_ADD, conn, &cev) == -1)
	{
		std::cerr << "epoll_ctl: " << strerror(errno) << "\n";
		// clients.erase(conn);
		return ;
	}
	std::cout << "Accepted a conn: " << conn << "\n";
}

int Multiplexer::loop(void)
{
	return 0;
}

// TODO: When the Multiplexer is constructed it should init all the current servers that have been

// parsed by the configuration.
void Multiplexer::init(void)
{
	// responsible for:
	//     - Creating the Epol instance. only.

	Multiplexer::epoll_fd = epoll_create(1024);
	if (Multiplexer::epoll_fd < 0)
	{
		throw std::runtime_error(std::strerror(errno));
	}
	// TODO: Register the server.
}

// TODO: When the Multiplexer is deconstructed it shuld actually make sure that every Server was dealloated successfully
// it should also remove the allocated epoll instance. then exit cleanly
void Multiplexer::deinit(void)
{
	// responsible for:
	//     - Decreating the epol instance, cleanup any other resources.
}

Server *Multiplexer::register_server(void)
{
	// responsible for:
	//     - creates a new listening socket, assigning it to a server,
	//       registring the socket in epol, adding the server with 0 clients.
	//     - adding server handler function
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

    std::cout << "HTTP/1.0 Server listening on localhost:8080\n";
	event.events = EPOLLIN;
	event.data.ptr = (&server);
	int code = epoll_ctl(SocketContext::epoll_fd, EPOLL_CTL_ADD, server.get_socket(), &event);
	if (code < 0)
	{
        std::cerr << "Epoll ctl failed\n";
        return (NULL); // TODO: raise an exception if this shit fails, do not return an error code
	}
	Multiplexer::servers[server_fd] = std::make_pair(Server(), Clients());
	Multiplexer::servers[server_fd].first
		.set_socket(server_fd);
	return (&Multiplexer::servers[server_fd].first);
}

Client *Multiplexer::register_client(int server_fd, int client_fd)
{
	// responsible for:
	//     - Accepting the connection.
	//     - adding client handler,
	//       registring the socket in epol, adding the server with 0 clients.
	Multiplexer::servers[server_fd].second[client_fd] = Client(client_handler);
	Multiplexer::servers[server_fd].second[client_fd].set_socket(client_fd);
	Multiplexer::servers[server_fd].second[client_fd].set_owner(server_fd);
	return (&Multiplexer::servers[server_fd].second[client_fd]);
}
