#include <Server.hpp>
#include <cctype>
#include <cstdlib>
#include <iostream>
#include <sys/epoll.h>
#include <sys/socket.h>

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

void Client::unchunkify_buffer(void)
{
	HttpRequest  &request = this->getParser().getRequestObject().getHttpRequest();
	ChunkContext &chunk = request.chunked_context;
	size_t       hex_size;
	
	if (!request.ischunked)
		return ; // Not chunked, nothing to do.

	if (request.chunked_context.status == CHUNK_START || request.chunked_context.status == CHUNK_SIZE) {
		hex_size = 0;
		while (isxdigit(this->_buffer[hex_size]))
			chunk.hex.push_back(this->_buffer[hex_size++]);
		chunk.strip_delimeter(this->_buffer, CHUNK_DATA, hex_size);
		if (chunk.status == CHUNK_DATA)
			chunk.convert_remaining_into_hex();
	}
	if (request.chunked_context.status == CHUNK_DATA) {
		if (chunk.remaining == 0) {
			chunk.status = CHUNK_COMPLETE;
			return ;
		}
		size_t to_copy = std::min(chunk.remaining, this->_buffer.size());
		chunk.remaining -= to_copy;
	}
}

void Client::parse_request() __THROWS_STRERROR
{
	Multiplexer *self;
	ssize_t      count;
	char         buff[READ_CHUNK_SIZE];

	int conn = this->get_socket();
	HttpParser &clientP = this->getParser();
	Cgi  &cgi_instance = this->response.get_resource_ref().cgi;

	self = Multiplexer::get_multiplexer(NULL);
	if (!self) throw "Well, failed to get a Multiplexer class";

	try {
		count = Multiplexer::read(conn, buff, READ_CHUNK_SIZE); // NOTE: If a read fails it should throw,
		push_into_buffer(this->_buffer, buff, count); // Read..

		if (clientP.state() == READY)
			this->unchunkify_buffer(); // Unchunkify if needed..
		if (this->response.getstage() == SendingResource) {
			if (this->_buffer.size() >= READ_CHUNK_SIZE)
				this->switch_mode(WRITING);
		} else if (this->response.getstage() == ProcessingCgi) {	
			cgi_instance.append_into_cgi_body_buffer(this->_buffer.data(), count);
			if (cgi_instance.client_done)
				this->switch_mode(WRITING);
		} else {
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
