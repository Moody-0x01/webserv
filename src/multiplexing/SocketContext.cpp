#include <Server.hpp>

SocketContext::SocketContext() : request_buffer(""), response_buffer(""), action(NULL), _sockfd(-1), _owner(-1), _owns_fd(true)
{
	this->parserInstance.setParent(this);
}
SocketContext::SocketContext(SocketHandler a, int sock) : request_buffer(""), response_buffer(""), action(a), _sockfd(sock), _owner(-1), _owns_fd(true)
{
	this->parserInstance.setParent(this);
}
SocketContext::SocketContext(SocketHandler a) : request_buffer(""), response_buffer(""), action(a), _sockfd(-1), _owner(-1), _owns_fd(true)
{
	this->parserInstance.setParent(this);
}

SocketContext::~SocketContext()
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

SocketContext &SocketContext::operator=(const SocketContext &Other)
{
	if (this != &Other)
	{
		this->_owner = Other.get_owner();
		this->_sockfd = Other.get_socket();
		this->_owns_fd = true;
		this->action = Other.action;
		// NOTE: Call disown on other after using it to
		// copy because c++98 has no move semantic

		this->request_buffer = Other.request_buffer;
		this->response_buffer = Other.response_buffer;
		this->parserInstance = Other.parserInstance;
		this->parserInstance.setParent(this);
	}
	return (*this);
}

void SocketContext::set_socket(int sockfd) { _sockfd = sockfd; }
int SocketContext::get_socket(void) const { return _sockfd; }
void SocketContext::set_owner(int owner) { _owner = owner; }
int SocketContext::get_owner(void) const { return _owner; }
Server *SocketContext::get_server(void) const { return &Multiplexer::servers[_owner].first; }
void SocketContext::disown(void) { _owns_fd = false; }

void SocketContext::free()
{
	Multiplexer::unregister_client(this->get_owner(), this->get_socket());
}

HttpParser &SocketContext::getParser()
{
	return this->parserInstance;
}
