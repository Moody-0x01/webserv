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
	Multiplexer *self;

	if (!_owns_fd)
		return ;
	self = Multiplexer::get_multiplexer(NULL);
	if (!self) throw "Well, failed to get a Multiplexer class";
	if (_sockfd != -1)
	{
		epoll_ctl(self->epoll_fd,
			EPOLL_CTL_DEL,
			this->_sockfd,
			NULL);
		close(_sockfd);
	}
	_sockfd = -1;
	_owns_fd = false;
}

void ASocketContext::disown(void)
{
	this->_owns_fd = false;
}

void ASocketContext::take_ownership(ASocketContext *Other)
{
	if (this != Other)
	{
		this->_sockfd = Other->get_socket();
		this->_owns_fd = true;
		Other->disown();
	}
}

void ASocketContext::set_socket(int sockfd) { _sockfd = sockfd; }
int ASocketContext::get_socket(void) const { return _sockfd; }
