#include "HTTP/Response.hpp"
#include "Multiplexing/ASocketContext.hpp"
#include <Server.hpp>
#include <cassert>
#include <cctype>
#include <cstdio>
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

Client::Client() : ASocketContext(), parserInstance(), response(), _owner(-1), ip(""), port("")
{
	this->last_event_time = 0;
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

bool Client::unchunkify_buffer(void)
{
	HttpRequest  &request = this->getParser().getRequestObject().getHttpRequest();
	ChunkContext &chunk = request.chunked_context;
	
	if (!request.ischunked) return (true);
	chunk.unpack(this->_buffer);
	if (chunk.is_corrupted())
	{
		this->response.set_status(BadRequest);
		return (false);
	}
	return (true);
}

void Client::read_into_request_buffer(void) __THROWS_STRERROR {
	char         buff[READ_CHUNK_SIZE];
	ssize_t      count, to_read;
	HttpParser   &clientP = this->getParser();
	HttpRequest  &request = this->getParser().getRequestObject().getHttpRequest();
	ChunkContext &chunk   = request.chunked_context;

	if (clientP.state() == READY && request.ischunked)
	{
		if (chunk.is_reading_size() || chunk.remaining == 0)
			to_read = (size_t)READ_CHUNK_SIZE;
		else
			to_read = std::min((size_t)READ_CHUNK_SIZE, chunk.remaining);
		count = Multiplexer::read(this->get_socket(), buff, to_read); // NOTE: If a read fails it should throw,
	} else
		count = Multiplexer::read(this->get_socket(), buff, READ_CHUNK_SIZE); // NOTE: If a read fails it should throw,
	push_into_buffer(this->_buffer, buff, count); // Read..
	if (clientP.state() == READY) {
		this->body_size += count;
		if (this->body_size > this->response.getmaxbodysize())
		{
			this->response.set_status(BadRequest);
			this->switch_mode(WRITING);
			return ;
		}
	}
	if (clientP.state() != READY)
		clientP.handle();
}

void Client::parse_request() __THROWS_STRERROR
{
	HttpRequest  &request = this->getParser().getRequestObject().getHttpRequest();
	ChunkContext &chunk   = request.chunked_context;
	HttpParser   &clientP = this->getParser();
	Cgi          &cgi_instance = this->response.get_resource_ref().cgi;

	this->read_into_request_buffer();
	if (clientP.state() != READY)
		return ;

	this->unchunkify_buffer();
	if (this->response.getstage() == Setup)
		this->response.setup(request);
	if (this->response.getstage() == ProcessingPost) {
		// this->switch_mode(READING);
		// TODO: Not implemented yet
		// Post function will be placed here. once it is done. then we switch to Post in order to return the 
		// result to the client.
		// this->response.process_post(request); // Note that is the function that will process post.
		// if (request.ischunked && request.chunked_context.is_done())
		// 	this->switch_mode(WRITING);
		this->_buffer.clear();
	}
	if (this->response.getstage() == ProcessingCgi)
	{
		if (this->getParser().getRequestObject().getMethod() != "POST")
		{
			this->switch_mode(WRITING);
			return ;
		}
		if (this->_buffer.size()) {
			cgi_instance.append_into_cgi_body_buffer(this->_buffer.data(), this->_buffer.size(), 
					(ChunkContext*)(request.ischunked * (uint64_t)&chunk));
			this->_buffer.clear();
		}
		if (cgi_instance.client_done) this->switch_mode(WRITING);
	}
	if (this->response.getstage() == SendingResource)
		this->switch_mode(WRITING);
}

void Client::generate_response(void) __THROWS_STRERROR
{
	// Cgi  &cgi_instance = this->response.get_resource_ref().cgi;
	HttpRequest &request = this->getParser().getRequestObject().getHttpRequest();

	response_stage_t &s = this->response.getstage();
	switch(s)
	{
		case Setup:
		case DoneSending: {
			this->free();
			return ;
		} break;
		case ProcessingCgi: {
			// Note: Here the cgi is done reading from the client and is activally trying to send the response from cgi..
			// So it gets data from cgi and forwards it to the client.
			this->response.process_cgi_instance(request);
		} break;
		case ProcessingPost: {
			// this->response.process_post(request);
			// Not implemented yet..
			// Responsible: Kooneo
			assert(0 && "Not implemented yet by `Kooneo`");
		} break;
		case SendingResource: {
			this->response.send_resource(request);
		} break;
	}
	if (s == DoneSending) this->free();
}

bool Client::timeout(void)
{
	time_t now;

	if (this->last_event_time == 0)
		return (false);
	now = time(NULL);
	if (now < 0)
		return (true);
	return (now - this->last_event_time) >= CLIENT_TIMEOUT;
}

void Client::action(uint32_t e) __THROWS_STRERROR
{
	this->last_event_time = time(NULL);
	try {
		if (e & EPOLLIN) {
			// std::cout << "Client EPOLLIN event\n";
			this->parse_request();
			return ;
		}
		if ((e & EPOLLOUT) || (e & EPOLLRDHUP))
		{
			// std::cout << "Client EPOLLOUT event\n";
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
