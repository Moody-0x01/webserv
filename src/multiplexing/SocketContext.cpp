#include <Multiplexing/SocketContext.hpp>

int SocketContext::epoll_fd = 0;

SocketContext::SocketContext(): request_buffer(""), response_buffer(""), action(NULL), _sockfd(-1), _owner(-1)
{
}

SocketContext::SocketContext(SocketHandler a, int sock): request_buffer(""), response_buffer(""), action(a), _sockfd(sock), _owner(-1)
{
}

SocketContext::SocketContext(SocketHandler a): request_buffer(""), response_buffer(""), action(a), _sockfd(-1), _owner(-1)
{
}

SocketContext::~SocketContext()
{
	if (_sockfd != -1) {
		close(_sockfd);
		std::cout << "Closed -> " << _sockfd << "\n";
	}
	_sockfd = -1;
}
