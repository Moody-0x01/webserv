#include <Server.hpp>
#include <cctype>
#include <stdint.h>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

HttpParser &Client::getParser(void) {
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
	if (!epoll_switch(self->epoll_fd, this->get_socket(), mode, this))
		throw strerror(errno);
}

void Client::unchunkify_buffer(void)
{
	HttpRequest  &request = this->getParser().getRequestObject().getHttpRequest();
	ChunkContext &chunk = request.chunked_context;
	
	if (!request.ischunked)
		return ;
	chunk.unpack(this->_buffer);
}

void Client::read_into_request_buffer(void) __THROWS_STRERROR {
	char         buff[READ_CHUNK_SIZE];
	ssize_t      count, to_read;
	HttpParser   &clientP = this->getParser();
	HttpRequest  &request = this->getParser().getRequestObject().getHttpRequest();
	ChunkContext &chunk   = request.chunked_context;

	if (clientP.state() == READY && request.ischunked)
	{
		if (chunk.is_reading_size())
			to_read = (size_t)READ_CHUNK_SIZE;
		else
			to_read = std::min((size_t)READ_CHUNK_SIZE, chunk.remaining);
		count = Multiplexer::read(this->get_socket(), buff, to_read); // NOTE: If a read fails it should throw,
		chunk.remaining -= (count * chunk.is_reading_data()); // Haha smart
		return ;
	} else
		count = Multiplexer::read(this->get_socket(), buff, READ_CHUNK_SIZE); // NOTE: If a read fails it should throw,

	push_into_buffer(this->_buffer, buff, count); // Read..
	// std::cout << "Read " << count << " bytes from client\n";
	if (clientP.state() != READY)
	{
		clientP.handle();
		// for (size_t i = 0; i < this->_buffer.size(); i++)
		// 	print_char_as_hex(this->_buffer[i]);
	}
	if (clientP.state() == READY) 
	{
		this->unchunkify_buffer(); // Unchunkify if needed..
		this->switch_mode(WRITING);
	}
}

void Client::parse_request() __THROWS_STRERROR
{
	HttpRequest  &request = this->getParser().getRequestObject().getHttpRequest();
	ChunkContext &chunk   = request.chunked_context;
	// HttpParser &clientP = this->getParser();
	Cgi  &cgi_instance = this->response.get_resource_ref().cgi;

	try {
		this->read_into_request_buffer();
		if (this->response.getstage() == SendingResource)
			this->switch_mode(WRITING);
		else if (this->response.getstage() == ProcessingCgi) {
			cgi_instance.append_into_cgi_body_buffer(this->_buffer.data(), this->_buffer.size(), 
					(ChunkContext*)(request.ischunked * (uint64_t)&chunk));
			if (cgi_instance.client_done || chunk.status_mask & CHUNK_COMPLETE)
			{
				std::cout << "Client is done with cgi body\n";
				this->switch_mode(WRITING);
			}
		} else
			this->switch_mode(WRITING);
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
		this->response
			.continue_processing(request);
		if (this->response.getstage() == DoneSending) {
			this->free();
			return ;
		}
		if (request.method == "POST")
		{
			if (request.ischunked && !request.chunked_context.is_done())
				this->switch_mode(READING);
			if (this->response.getstage() == ProcessingCgi && !cgi_instance.client_done)
			{
				this->switch_mode(READING); // switch_mode to Reading body from the client.
											// any kind of post needs to go back to recv switch_mode
			}
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

void Client::setport(std::string port)
{
	this->port = port;
}

std::string Client::getport(void)
{
	return (this->port);
}

void Client::setip_from_bytes(uint32_t ip_bytes)
{
	HttpRequest r = this->getParser().getRequestObject().getHttpRequest();
	std::stringstream ss;

    ss << ((ip_bytes >> 24) & 0xFF) << "."
       << ((ip_bytes >> 16) & 0xFF) << "."
       << ((ip_bytes >> 8)  & 0xFF) << "."
       << ((ip_bytes >> 0)  & 0xFF);
    this->setip(ss.str());
	r.headers["REMOTE_ADDR"] = this->getip();
}

void Client::setport_from_bytes(uint32_t port_bytes)
{
	HttpRequest r = this->getParser().getRequestObject().getHttpRequest();
	std::stringstream ss;

    ss << ((port_bytes >> 24) & 0xFF) << "."
       << ((port_bytes >> 16) & 0xFF) << "."
       << ((port_bytes >> 8)  & 0xFF) << "."
       << ((port_bytes >> 0)  & 0xFF);
    this->setport(ss.str());
	r.headers["REMOTE_PORT"] = this->getport();
}
