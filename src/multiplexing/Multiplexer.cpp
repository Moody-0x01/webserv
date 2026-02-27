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

void client_handler(uint32_t e, Client *Self)
{
	const char* http10_ok_header =
		"HTTP/1.0 200 OK\r\n"
		"Content-Type: text/plain\r\n";
	char buff[4096];
	int conn;
	ssize_t count;

	conn = Self->get_socket();
	if (e & EPOLLOUT) {
		std::cout << "Writing to conn: " << conn << "\n";;
		count = read(conn, buff, sizeof(buff));
		if (count > 0) Self->request_buffer += std::string(buff);
		Self->response_buffer = http10_ok_header + Self->request_buffer;
		count = write(conn, Self->response_buffer.c_str(), Self->response_buffer.size());
		if (count == -1)
			std::cerr << "write: " << strerror(errno) << "\n";
		Multiplexer::unregister_client(Self->get_owner(), Self->get_socket());
	} else if (e & EPOLLIN) {
		std::cout << "Reading from conn: " << conn << "\n";;
		count = read(conn, buff, sizeof(buff));
		std::cout << "Read: " << count << "\n";
		if (count > 0) {
			Self->request_buffer += std::string(buff);
		} else if (count <= 0) {
			if (count < 0)
				std::cerr << "read: " << strerror(errno) << "\n";
			Multiplexer::unregister_client(Self->get_owner(), Self->get_socket());
			return ;
		}
		if (Self->request_buffer.find("\r\n\r\n") != std::string::npos)
		{
			struct epoll_event cev;
			cev.events = EPOLLOUT;
			cev.data.ptr = Self;
			if (epoll_ctl(Multiplexer::epoll_fd, EPOLL_CTL_MOD, conn, &cev) ==
			-1) {
				std::cerr << "epoll_ctl: " << strerror(errno) << "\n";
				Multiplexer::unregister_client(Self->get_owner(), Self->get_socket());
			}
			std::cout << "Client " << conn << " is done sending http/1.0\n";
		}
	}
}

void server_handler(uint32_t e, Server *Self)
{
	Client *conn;

	conn = Multiplexer::register_client(e, Self);
	struct epoll_event cev;
	cev.events = EPOLLIN;
	cev.data.ptr = conn;
	std::cout << "Accepted a conn: " << conn << "\n";
	if (epoll_ctl(Multiplexer::epoll_fd, EPOLL_CTL_ADD, conn->get_socket(), &cev) == -1)
	{
		Multiplexer::servers[Self->get_socket()].second.erase(conn->get_socket());
		throw std::runtime_error(strerror(errno));
	}
}

int Multiplexer::loop(void)
{
	return 0;
}

// TODO: When the Multiplexer is constructed it should init all the current servers that have been

// parsed by the configuration.
void Multiplexer::init(void)
{
	Multiplexer::epoll_fd = epoll_create(1024);
	if (Multiplexer::epoll_fd < 0)
		throw std::runtime_error(std::strerror(errno));
	std::cout << "EPOL: " << Multiplexer::epoll_fd  << "\n";
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
	Multiplexer::servers[server_fd] = std::make_pair(server, Clients());
	server.disown(); // So it does not close the socket at exit

    std::cout << "HTTP/1.0 Server listening on localhost:8080\n";
	event.events = EPOLLIN;
	event.data.ptr = &Multiplexer::servers[server_fd].first;
	int code = epoll_ctl(Multiplexer::epoll_fd, EPOLL_CTL_ADD, server.get_socket(), &event);
	if (code < 0)
	{
		Multiplexer::servers.erase(server_fd);
		return (NULL); // TODO: raise an exception if this shit fails, do not return an error code
	}
	return ((Server*)event.data.ptr);
}

Client *Multiplexer::register_client(uint32_t e, Server *server)
{
	// responsible for:
	//     - Accepting the connection.
	//     - adding client handler,
	//       registring the socket in epol, adding the server with 0 clients.
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
	if (!set_nonblocking(conn)) return (NULL);
	Multiplexer::servers[server->get_socket()].second[conn] = client;
	client.disown();
	return (&Multiplexer::servers[server->get_socket()].second[conn]);
}
