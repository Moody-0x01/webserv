# include <Server.hpp>

int main() {	 
	Multiplexer::init();
    while (true)
	{
		int ready = epoll_wait(Multiplexer::epoll_fd,
						 Multiplexer::events, EVENT_MAX, 100);
		if (ready < 0)
		{
			std::cerr << "epoll_wait: " << strerror(errno) << "\n";
			return 1;
		}
		for (int index = 0; index < ready; ++index)
		{
			SocketContext *handle = (SocketContext *)(Multiplexer::events[index].data.ptr);
			std::cout << "Next: " << handle->get_socket() << "\n";
			handle->action(Multiplexer::events[index].events, handle);
		}
    }
	Multiplexer::deinit();
    return 0;
}
