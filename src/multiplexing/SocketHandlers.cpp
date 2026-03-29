#include <Server.hpp>
#include <sys/epoll.h>
#include <unistd.h>

void client_request(Client *client) __THROWS_STRERROR
{
	int conn = client->get_socket();
	// ServerConfig &config;
	HttpParser &clientP = client->getParser();
	char buff[READ_CHUNK_SIZE];

	try {
		ssize_t count = Multiplexer::read(conn, buff, READ_CHUNK_SIZE); // NOTE: If a read fails it should throw,
		std::cout << "Read: " << count << "\n";
		client->request_buffer.append(buff, count);
		clientP.handle();
		if (clientP.state() == READY)
		{
			struct epoll_event cev;
			cev.events = EPOLLOUT | EPOLLHUP | EPOLLERR;
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

void client_response(Client *client) __THROWS_STRERROR
{
	// TODO: ok! Now i have got a request. it needs to be processed that way.
	// An example of the request:
	// <Method> <URI> <HTTP-Version>\r\n
	// <Header-Name>: <Header-Value>\r\n
	// ...
	// \r\n
	// <optional body>
	// Response
	// Response R(client->request);
	// Now: handling the response and saving its state will be done by using this field. client->response
	
	std::string http10_ok_header =
		"HTTP/1.0 200 OK\r\n"
		"Content-Type: text/html\r\n\r\n";
	int conn = client->get_socket();
	try {
		client->response_buffer = http10_ok_header + "<p style='background: #191919; color: white;'> Hello from server !";
		Multiplexer::write(conn, client->response_buffer.c_str(), client->response_buffer.size()); // NOTE: if a write fails,
		int out = dup(STDOUT_FILENO);
		dup2(conn, STDOUT_FILENO);
		{
			printf("%s:%s\n", Multiplexer::confs[client->get_owner()].host.c_str(), Multiplexer::confs[client->get_owner()].port.c_str());
		}
		dup2(out, STDOUT_FILENO);
		Multiplexer::write(conn, "</p>", 4); // NOTE: if a write fails,
		client->free();
	} catch (const char *e) {
		client->free();
		throw e;
	}
}

void client_handler(uint32_t e, Client *Self) __THROWS_STRERROR
{
	// NOTE(1): Any syscall that fails here should raise an exception;
	// NOTE(2): Well gotta handle those too
	/*  EPOLLOUT    */
	/*  EPOLLERR    */
	/*  EPOLLHUP  : Client complete closed the connexion. */
	/*  EPOLLRDHUP: Client finished sending data. */
	if (e & (EPOLLERR | EPOLLHUP))
    {
        Self->free(); return ;
    }

	try {
    if (e & EPOLLIN)
        client_request(Self);
    if (e & EPOLLOUT)
        client_response(Self);
    if (e & EPOLLRDHUP)
        client_response(Self);
	} catch (const char *e) {
		throw e;
	}
}

void server_handler(uint32_t e, Server *Self) __THROWS_STRERROR
{
	Client *conn;
	struct epoll_event cev;

	conn = Multiplexer::register_client(e, Self);
	cev.events = EPOLLIN | EPOLLRDHUP | EPOLLERR;
	cev.data.ptr = conn;
	std::cout << "Accepted a conn: " << conn << "\n";
	if (epoll_ctl(Multiplexer::epoll_fd, EPOLL_CTL_ADD, conn->get_socket(), &cev) == -1)
	{
		Multiplexer::servers[Self->get_socket()].second.erase(conn->get_socket());
		throw strerror(errno);
	}
}
