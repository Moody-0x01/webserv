#include <Server.hpp>
#include <iostream>
#include <sys/epoll.h>
#include <sys/socket.h>

HttpParser &Client::getParser(void)
{
	return this->parserInstance;
}

Client::~Client()
{
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
	Multiplexer::unintroduce_context((uint64_t)this);
	std::cout << "Client freed!\n";
}

void Client::switch_mode(socket_mode_t mode) __THROWS_STRERROR
{
	Multiplexer *self;
	self = Multiplexer::get_multiplexer(NULL);
	if (!self) throw "Well, failed to get a Multiplexer class";
	if (!epoll_switch(self->epoll_fd, this->get_socket(), mode, this))
		throw strerror(errno);
}

void Client::parse_request() __THROWS_STRERROR
{
	int conn = this->get_socket();
	HttpParser &clientP = this->getParser();
	Cgi  &cgi_instance = this->response.get_resource_ref().cgi;
	Multiplexer *self;
	char buff[READ_CHUNK_SIZE];

	self = Multiplexer::get_multiplexer(NULL);
	if (!self) throw "Well, failed to get a Multiplexer class";

	try {
		ssize_t count = Multiplexer::read(conn, buff, READ_CHUNK_SIZE); // NOTE: If a read fails it should throw,

		if (this->response.getstage() == SendingResource) {
			push_into_buffer(this->_buffer, buff, count);
			if (this->_buffer.size() >= READ_CHUNK_SIZE * 2)
				this->switch_mode(WRITING);
		} else if (this->response.getstage() == ProcessingCgi) {	
			cgi_instance.append_into_cgi_body_buffer(buff, count);
			if (cgi_instance.client_done)
				this->switch_mode(WRITING);
		} else {
			push_into_buffer(this->_buffer, buff, count);
			clientP.handle();
			if (clientP.state() == READY)
				this->switch_mode(WRITING);
		}
	} catch (const char *e) {
		this->free();
		throw e;
	}
}

void Client::generate_response(void) __THROWS_STRERROR
{
	Cgi  &cgi_instance = this->response.get_resource_ref().cgi;
	HttpRequest &request = this->getParser().getRequestObject().getHttpRequest();

	try {
		// std::cout <<  "At continue_processing with:  " << request.method << "\n";
		request.headers["REMOTE_ADDR"] = this->ip;
		this->response
			.continue_processing(request);
		if (this->response.getstage() == DoneSending) {
			this->free();
			return ;
		}
		if (request.method == "POST" && !cgi_instance.client_done)
		{
			this->switch_mode(READING); // switch_mode to Reading body from the client.
										// any kind of post needs to go back to recv mode
			return ;
		}
	} catch (const char *e) {
		this->free();
		throw e;
	}
}

void Client::action(uint32_t e) __THROWS_STRERROR
{
	try {
    if (e & EPOLLIN) {
		// std::cout << "Client::action::EPOLLIN\n";
		this->parse_request();
		return ;
	}
    if ((e & EPOLLOUT) || (e & EPOLLRDHUP))
	{
		// std::cout << "Client::action::EPOLLOUT\n";
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
