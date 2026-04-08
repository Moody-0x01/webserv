#include <Server.hpp>

ASocketContext::ASocketContext() : request_buffer(""), response_buffer(""), _sockfd(-1), _owns_fd(true)
{
}

ASocketContext::ASocketContext(int sock) : request_buffer(""), response_buffer(""), _sockfd(sock), _owns_fd(true)
{
}

ASocketContext::~ASocketContext()
{
	if (!_owns_fd)
		return ;
	if (_sockfd != -1)
	{
		std::cout << "Closed: " << _sockfd << "\n";
		close(_sockfd);
	}
	_sockfd = -1;
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
