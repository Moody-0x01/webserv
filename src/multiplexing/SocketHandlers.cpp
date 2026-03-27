#include <Server.hpp>
#include <sys/epoll.h>
#include <unistd.h>

void client_request(Client *client) __THROWS_STRERROR
{
	int conn = client->get_socket();
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
	// NOTE: The work is here, I need to create a meaningful and straigh forward way to handle http methods.
	// but the order of this is as follows.
	// 1 - check the validity of the request. if it is not valid then throw something meaningful to the client instead of going forward.
	// 2 - if the request is trying to get something. then go to the root of the server and look for it.
	//     2.1 - if it is a dir, then list it and forward the listing to the client.
	//     2.2 - if it is a file, then serve the file. generate a mime type then hande it over.
	//     2.3 - if it is not found, then return 404.html as a backup, and if any syscall fails then then return 5xx.html
	// 3 - if the request is trying to post something, then get the mime type.
	Response generated;
	
	std::string http10_ok_header =
		"HTTP/1.0 200 OK\r\n"
		"Content-Type: text/html\r\n\r\n";
	int conn = client->get_socket();
	// Response
	std::cout << "Writing to conn: " << conn << "\n";
	try {
		client->response_buffer = http10_ok_header + "<p style='background: #191919; color: white;'> Hello from server !";
		Multiplexer::write(conn, client->response_buffer.c_str(), client->response_buffer.size()); // NOTE: if a write fails,
		int out = dup(STDOUT_FILENO);
		dup2(conn, STDOUT_FILENO);
		{
			Config c;
			c.addServer(client->get_server()->conf);
			c.debug();
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
