# include <Server.hpp>

void Server::action(uint32_t e) __THROWS_STRERROR
{
	Client *conn;
	struct epoll_event cev;

	conn = Multiplexer::register_client(e, this);
	cev.events = EPOLLIN | EPOLLRDHUP | EPOLLERR;
	cev.data.ptr = conn;
	std::cout << "Accepted a conn: " << conn << "\n";
	if (epoll_ctl(Multiplexer::epoll_fd, EPOLL_CTL_ADD, conn->get_socket(), &cev) == -1)
	{
		Multiplexer::servers[this->get_socket()].second.erase(conn->get_socket());
		throw strerror(errno);
	}
}
