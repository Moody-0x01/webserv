#include <Server.hpp>
#include <cstddef>
#include <sys/epoll.h>

ASocketContext::ASocketContext() : request_buffer(""), response_buffer(""), _sockfd(-1), _owns_fd(true)
{
}

ASocketContext::ASocketContext(int sock) : request_buffer(""), response_buffer(""), _sockfd(sock), _owns_fd(true)
{
}

ASocketContext::~ASocketContext()
{

	if (!_owns_fd || _sockfd == -1)
		return ;
	unregister_fd(_sockfd);
	_sockfd = -1;
	_owns_fd = false;
}

void ASocketContext::disown(void)
{
	this->_owns_fd = false;
}

void ASocketContext::set_socket(int sockfd) { _sockfd = sockfd; }
int ASocketContext::get_socket(void) const { return _sockfd; }
