#include <Server.hpp>

HttpParser &Client::getParser(void)
{
	return this->parserInstance;
}

void Client::set_owner(int owner) { _owner = owner; }
int Client::get_owner(void) const { return _owner; }

Server *Client::get_server(void) const __THROWS_STRERROR {
	Multiplexer *self;
	self = Multiplexer::get_multiplexer(NULL);
	if (!self) throw "Well, failed to get a Multiplexer class";
	return self->get_owner(this->get_socket());
}

void Client::take_ownership(ASocketContext *Other)
{
	ASocketContext::take_ownership(Other);
	Client *realOther = dynamic_cast<Client*>(Other);

    if (realOther) {
        // Now you have access to Client-specific fields!
        this->_owner = realOther->_owner;
        this->parserInstance = realOther->parserInstance;
		this->parserInstance.setParent(this);
    }
}

void Client::free()
{
	Multiplexer::unregister_client(this->get_owner(), this->get_socket());
}

void Client::parse_request() __THROWS_STRERROR
{
	int conn = this->get_socket();
	Multiplexer *self;
	struct epoll_event cev;

	self = Multiplexer::get_multiplexer(NULL);
	if (!self) throw "Well, failed to get a Multiplexer class";
	HttpParser &clientP = this->getParser();
	char buff[READ_CHUNK_SIZE];

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
		{
			this->free();
			std::cout << "DONE!!!\n";
		}
	} catch (const char *e) {
		this->free();
		throw e;
	}
}


void Client::action(uint32_t e) __THROWS_STRERROR
{
	if (e & (EPOLLERR | EPOLLHUP))
		throw "";
	try {
    if (e & EPOLLIN) this->parse_request();
    if ((e & EPOLLOUT) || (e & EPOLLRDHUP))
        this->generate_response();
	} catch (const char *e) {
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
