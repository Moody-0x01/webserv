#include "HTTP/Response.hpp"
#include <Server.hpp>
#include <cassert>
#include <cctype>
#include <cstdio>
#include <stdint.h>
#include <cstdlib>
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
	this->last_event_time = time(NULL);
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
	Multiplexer::unintroduce_context((uintptr_t)this);
	Multiplexer::erase_context((uintptr_t)this);
	std::cout << "Client freed!\n";
}

void Client::switch_mode(socket_mode_t mode) __THROWS_STRERROR
{
	Multiplexer *self;
	self = Multiplexer::get_multiplexer(NULL);
	if (!utility::epoll_switch(self->epoll_fd, this->get_socket(), mode, this))
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
	utility::push_into_buffer(this->_buffer, buff, count); // Read..
	if (clientP.state() == READY) {
		return ;
	}
	clientP.handle();
}

void Client::parse_request() __THROWS_STRERROR
{
	HttpRequest  &request = this->getParser().getRequestObject().getHttpRequest();
	ChunkContext &chunk   = request.chunked_context;
	HttpParser   &clientP = this->getParser();
	Cgi          &cgi_instance = this->response.get_resource_ref().cgi;

	this->read_into_request_buffer();
	if (clientP.state() != READY) return ;

	this->unchunkify_buffer();
	if (this->response.getstage() == Setup) {
		this->response.setup(request);
	} else {
		switch (this->response.getstage())
		{
			case ProcessingPost: {
				this->response
					.dump_post_body(this->_buffer);
			} break;
			case ProcessingCgi: {
				cgi_instance.append_into_cgi_body_buffer(this->_buffer.data(), this->_buffer.size(), 
						(ChunkContext*)(request.ischunked * (uintptr_t)&chunk));
				this->_buffer.clear();
			} break;
			default: {} break;
		}
	}
	if (this->response.getstage() == ProcessingCgi && cgi_instance.client_done)  this->switch_mode(WRITING);
	if (this->response.getstage() == SendingResource)                            this->switch_mode(WRITING);
	if (this->getParser().getRequestObject().getMethod() != "POST")              this->switch_mode(WRITING);
}

void Client::generate_response(void) __THROWS_STRERROR
{
	HttpRequest &request = this->getParser().getRequestObject().getHttpRequest();
	response_stage_t &s = this->response.getstage();

	switch(s)
	{
		case Setup:
		case ProcessingPost:
		case DoneSending: {
			this->free();
			return ;
		} break;
		case ProcessingCgi: {
			this->response
				.process_cgi_instance(request);
			if (s == SendingResource) {
				this->response.send_resource(request);
			}
		} break;
		case SendingResource: {
			this->response.send_resource(request);
		} break;
	}
	if (s == DoneSending) this->free();
}

bool Client::timeout(void) __THROWS_STRERROR
{
	time_t now;

	if (this->last_event_time == 0)
		return (false);
	now = time(NULL);
	if (now < 0) return (true);
	return (now - this->last_event_time) >= CLIENT_TIMEOUT;
}

void Client::action(uint32_t e) __THROWS_STRERROR
{
	this->last_event_time = time(NULL);
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
	// static int i = 0;
	HttpRequest r = this->getParser().getRequestObject().getHttpRequest();
	std::stringstream ss;

	ss << port_bytes;
    this->setport(ss.str());
	r.headers["REMOTE_PORT"] = this->getport();
	// std::cout << "[" << i++ << "]" << this->getip() << ":" << this->getport() << "\n";
}
