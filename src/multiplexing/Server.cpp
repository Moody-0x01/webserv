# include <Server.hpp>
#include <iostream>

void Server::action(uint32_t e) __THROWS_STRERROR
{
	Client *conn;
	Multiplexer *self;
	struct epoll_event cev;

	self = Multiplexer::get_multiplexer(NULL);
	if (!self) throw "Well, failed to get a Multiplexer class";
	conn = Multiplexer::register_client(e, this);
	cev.events = EPOLLIN | EPOLLRDHUP | EPOLLERR;
	cev.data.ptr = conn;
	std::cout << "Accepted a conn: " << conn << "\n";
	if (epoll_ctl(self->epoll_fd, EPOLL_CTL_ADD, conn->get_socket(), &cev) == -1)
	{
		Multiplexer::unregister_client(this->get_socket(),
				conn->get_socket());
		throw strerror(errno);
	}
}
