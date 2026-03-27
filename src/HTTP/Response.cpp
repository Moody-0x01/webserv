#include <Server.hpp>
#include <algorithm>

std::map<int, std::string> Response::status_lines;

Response::Response()
{
}

Response::~Response()
{
}

void Response::serialize()
{
	// TODO: Doing this later.
}

bool Response::isdone(void)
{
	// TODO: What if the body was not sent yet??
	// what if it is a file? cgi?..
	return (this->__is_serialized && this->sent == this->__serialized_response.size());
}

void Response::write(int conn) __THROWS_STRERROR
{
	ssize_t count;
	size_t  write_size;

	if (!this->__is_serialized)
		this->serialize(); // NOTE: converts headers and body into client writable form in __serialized_response

	write_size = WRITE_CHUNK_SIZE;
	if (write_size > this->__serialized_response.size() - this->sent)
		write_size = this->__serialized_response.size() - this->sent;

	count = ::write(conn,
		this->__serialized_response.c_str() + this->sent,
		write_size);
	if (count <= 0) throw strerror(errno);
	this->sent += count;
	// TODO: Well, lazy loading files is probably better.
	// html files, audio, video files. should be loaded.
}

void Response::init_status_lines()
{
    Response::status_lines[OK                 ] = "HTTP/1.0 200 OK";
    Response::status_lines[Created            ] = "HTTP/1.0 201 Created";
    Response::status_lines[NoContent          ] = "HTTP/1.0 204 No Content";
    Response::status_lines[MovedPermanently   ] = "HTTP/1.0 301 Moved Permanently";
    Response::status_lines[Found              ] = "HTTP/1.0 302 Found";
    Response::status_lines[NotModified        ] = "HTTP/1.0 304 Not Modified";
    Response::status_lines[BadRequest         ] = "HTTP/1.0 400 Bad Request";
    Response::status_lines[Unauthorized       ] = "HTTP/1.0 401 Unauthorized";
    Response::status_lines[Forbidden          ] = "HTTP/1.0 403 Forbidden";
    Response::status_lines[NotFound           ] = "HTTP/1.0 404 Not Found";
    Response::status_lines[MethodNotAllowed   ] = "HTTP/1.0 405 Method Not Allowed";
    Response::status_lines[RequestTimeout     ] = "HTTP/1.0 408 Request Timeout";
    Response::status_lines[InternalServerError] = "HTTP/1.0 500 Internal Server Error";
    Response::status_lines[NotImplemented     ] = "HTTP/1.0 501 Not Implemented";
    Response::status_lines[BadGateway         ] = "HTTP/1.0 502 Bad Gateway";
    Response::status_lines[ServiceUnavailable ] = "HTTP/1.0 503 Service Unavailable";
}

void Response::appendheader(const std::string key, const std::string value)
{
	this->headers[key] = value + "\r\n";
}

void Response::appendbody(const std::string _body)
{
	this->body += _body;
}
