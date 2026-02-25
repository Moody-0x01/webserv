#include <Server.hpp>

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
