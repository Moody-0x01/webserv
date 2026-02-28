# include <Server.hpp>

void client_handler(uint32_t e, Client *Self)
{
	const char* http10_ok_header =
		"HTTP/1.0 200 OK\r\n"
		"Content-Type: text/plain\r\n\r\n";
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
		Multiplexer::unregister_client(
			Self->get_owner(),
			Self->get_socket());
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
