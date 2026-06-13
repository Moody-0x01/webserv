#include <Server.hpp>
#include <cstddef>
#include <sys/epoll.h>

ASocketContext::ASocketContext() : _sockfd(-1), _owns_fd(true)
{
}

ASocketContext::ASocketContext(int sock) : _sockfd(sock), _owns_fd(true)
{
}

ASocketContext::~ASocketContext()
{

	if (!_owns_fd || _sockfd == -1) return ;
	unregister_fd(_sockfd);
	_sockfd = -1;
	_owns_fd = false;
}

void ASocketContext::disown(void)
{
	this->_owns_fd = false;
}

bool ASocketContext::timeout(void)  __THROWS_STRERROR
{
	return (false);
}

void ASocketContext::free()
{
}

void ASocketContext::set_socket(int sockfd) { _sockfd = sockfd; }
int ASocketContext::get_socket(void) const { return _sockfd; }
