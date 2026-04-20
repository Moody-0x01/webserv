#include <Server.hpp>
#include <sys/socket.h>

HttpParser &Client::getParser(void)
{
	return this->parserInstance;
}

Client::~Client()
{
	std::cout << this->get_socket() << " was freeed::::)\n";
}

void Client::set_owner(int owner) { _owner = owner; }
int Client::get_owner(void) const { return _owner; }

Server *Client::get_server(void) const __THROWS_STRERROR {
	Multiplexer *self;
	self = Multiplexer::get_multiplexer(NULL);
	if (!self) throw "Well, failed to get a Multiplexer class";
	return self->get_owner(this->get_socket());
}


void Client::free()
{
	shutdown(this->get_socket(), SHUT_RDWR);
	Multiplexer::unregister_client(this->get_owner(),
				this->get_socket());
}

void Client::parse_request() __THROWS_STRERROR
{
	int conn = this->get_socket();
	Multiplexer *self;
	struct epoll_event cev;
	char buff[READ_CHUNK_SIZE];

	self = Multiplexer::get_multiplexer(NULL);
	if (!self) throw "Well, failed to get a Multiplexer class";
	HttpParser &clientP = this->getParser();

	try {
		ssize_t count = Multiplexer::read(conn, buff, READ_CHUNK_SIZE); // NOTE: If a read fails it should throw,
		if (this->response.get_resource_ref().cgi.state == WritingBody)
			this->response.get_resource_ref().cgi.append_into_body_buffer(buff, count);
		else {
		this->request_buffer.append(buff, count);
		clientP.handle();
		if (clientP.state() == READY)
		{
			cev.events = EPOLLOUT | EPOLLHUP | EPOLLERR;
			cev.data.ptr = this;
			if (epoll_ctl(self->epoll_fd, EPOLL_CTL_MOD, conn, &cev) == -1)
			{
				std::cerr << "epoll_ctl: " << strerror(errno) << "\n";
				this->free();
			}
		}
		}
	} catch (const char *e) {
		this->free();
		throw e;
	}
}

void Client::generate_response(void) __THROWS_STRERROR
{
	try {	
		HttpRequest &request = this->getParser().getRequestObject().getHttpRequest();
		request.headers["REMOTE_ADDR"] = this->ip;
		this->response
			.continue_processing(request);
		if (this->response.getstage() == DoneSending)
			this->free();
	} catch (const char *e) {
		this->free();
		throw e;
	}
}


void Client::action(uint32_t e) __THROWS_STRERROR
{
	/*  std::cout << this->get_socket() << " triggered an action\n";  */
	try {
    if (e & EPOLLIN) {
		this->parse_request();
		return ;
	}
    if ((e & EPOLLOUT) || (e & EPOLLRDHUP))
	{
		this->generate_response();
		return ;
	}
	if (e & EPOLLHUP)
	{
		// I can not read from cgi anymore. this is an internal server error and cgi should be marked as free
		// if the headers are not sent yet then we should send internal server error.
		// else just hangup and thas it.
		this->free();
		return ;
	}
	if (e & EPOLLRDHUP)
	{
		// I can not write body to connexion anymore..
		// if I did not send any heades then it makes sense to just send internal server error.
		this->free();
		return ;
	}

	if (e & EPOLLERR) {
		// Error !!
		this->free();
		return;
	}
	} catch (const char *e) {
		this->free();
		throw e;
	}
}

void Client::setip(std::string address)
{
	this->ip = address;
}

std::string Client::getip(void)
{
	return (this->ip);
}

void Client::setip_from_bytes(uint32_t ip_bytes)
{

	std::stringstream ss;

    ss << ((ip_bytes >> 24) & 0xFF) << "."
       << ((ip_bytes >> 16) & 0xFF) << "."
       << ((ip_bytes >> 8)  & 0xFF) << "."
       << ((ip_bytes >> 0)  & 0xFF);
    this->setip(ss.str());
}
