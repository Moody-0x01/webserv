# include <Server.hpp>
#include <iostream>
#include <sys/epoll.h>

void Server::action(uint32_t e) __THROWS_STRERROR
{
	Client *conn;
	Multiplexer *self;
	struct epoll_event cev;

	self = Multiplexer::get_multiplexer(NULL);
	if (!self) throw "Well, failed to get a Multiplexer class";
	if ((e & EPOLLERR) || (e & EPOLLHUP)) 
    {
        /* If this happens, the listening socket is dead. 
           Common causes: The FD was closed elsewhere, or a critical 
           kernel resource limit was hit.
        */
		// TODO: unregister_server.

		Multiplexer::unintroduce_context((uint64_t)this);
		self->servers
			.erase(this->get_socket());
        throw "Critical error on listening socket (EPOLLERR/EPOLLHUP)";
    }
	if (e & EPOLLIN) {
		conn = Multiplexer::register_client(e, this);
		cev.events = EPOLLIN | EPOLLRDHUP | EPOLLERR;
		cev.data.ptr = conn;
		if (epoll_ctl(self->epoll_fd, EPOLL_CTL_ADD, conn->get_socket(), &cev) == -1)
		{
			Multiplexer::unregister_client(this->get_socket(),
					conn->get_socket());
			throw strerror(errno);
		}
		Multiplexer::introduce_new_context((uint64_t)conn, true);
	}
}
