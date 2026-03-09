#include <Server.hpp>
#include <cstddef>
#include <unistd.h>

void client_request(Client *client)
{
	int conn = client->get_socket();
	HttpParser &clientP = client->getParser();
	char buff[BUFFER_SIZE];

	try {
		ssize_t count = Multiplexer::read(conn, buff, BUFFER_SIZE); // NOTE: If a read fails it should throw,
		std::cout << "Read: " << count << "\n";
		client->request_buffer.append(buff, count);
		clientP.handle();
		if (clientP.state() == READY)
		{
			struct epoll_event cev;
			cev.events = EPOLLOUT;
			cev.data.ptr = client;
			if (epoll_ctl(Multiplexer::epoll_fd, EPOLL_CTL_MOD, conn, &cev) == -1)
			{
				// NOTE: If an epoll_ctl fails it should throw. haha throw up something. whatever
				std::cerr << "epoll_ctl: " << strerror(errno) << "\n";
				client->free();
			}
		}
	} catch (const char *e) {
		client->free();
		throw e;
	}
}

void client_response(Client *client)
{
	std::string http10_ok_header =
		"HTTP/1.0 200 OK\r\n"
		"Content-Type: text/html\r\n\r\n";
	int conn = client->get_socket();
	// Response
	std::cout << "Writing to conn: " << conn << "\n";
	try {
		client->response_buffer = http10_ok_header + "<p style='background: #191919; color: white;'> Hello from server !";
		Multiplexer::write(conn, client->response_buffer.c_str(), client->response_buffer.size()); // NOTE: if a write fails,
		int out = dup(1);
		dup2(conn, 1);
		{
			Config c;
			c.addServer(client->get_server()->conf);
			c.debug();
		}
		dup2(out, 1);
		Multiplexer::write(conn, "</p>", 4); // NOTE: if a write fails,
		client->free();
	} catch (const char *e) {
		client->free();
		throw e;
	}
}

void client_handler(uint32_t e, Client *Self)
{
	// NOTE: Any syscall that fails here should raise an exception;
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
		throw strerror(errno);
	}
}
