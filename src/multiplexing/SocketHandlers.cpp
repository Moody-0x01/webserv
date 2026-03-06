#include <Server.hpp>

void client_request(Client *client)
{
	int conn = client->get_socket();
	HttpParser &clientP = client->getParser();
	char buff[BUFFER_SIZE];

	ssize_t count = read(conn, buff, BUFFER_SIZE);
	std::cout << "Read: " << count << "\n";
	if (count > 0)
	{
		client->request_buffer.append(buff, count);
		clientP.handle();
	}
	else
	{
		if (count < 0)
			std::cerr << "read: " << strerror(errno) << "\n";
		client->free();
		return;
	}

	if (clientP.state() == READY)
	{
		//  TODO: check the request code ... 
		struct epoll_event cev;
		cev.events = EPOLLOUT;
		cev.data.ptr = client;
		if (epoll_ctl(Multiplexer::epoll_fd, EPOLL_CTL_MOD, conn, &cev) == -1)
		{
			std::cerr << "epoll_ctl: " << strerror(errno) << "\n";
			client->free();
		}
	}
}

void client_response(Client *client)
{
	const char *http10_ok_header =
		"HTTP/1.0 200 OK\r\n"
		"Content-Type: text/plain\r\n\r\n";
	int conn = client->get_socket();
	char buff[BUFFER_SIZE];

	// Response
	std::cout << "Writing to conn: " << conn << "\n";
	ssize_t count = read(conn, buff, BUFFER_SIZE);
	if (count > 0)
		client->request_buffer.append(buff);
	client->response_buffer = http10_ok_header + client->request_buffer;
	count = write(conn, client->response_buffer.c_str(), client->response_buffer.size());
	if (count == -1)
		std::cerr << "write: " << strerror(errno) << "\n";
	client->free();
}

void client_handler(uint32_t e, Client *Self)
{
	if (e & EPOLLOUT)
		client_response(Self);
	else if (e & EPOLLIN)
		client_request(Self);
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
