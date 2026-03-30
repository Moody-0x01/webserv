#include <Server.hpp>

Request::Request() {};

Request::~Request() {};

const HttpRequest &Request::getHttpRequest(void) const
{
	return (this->request);
}

void Request::set_sockets(const int server, const int client)
{
	this->request.owner = server;
	this->request.conn  = client;
}

void Request::setMethod(const std::string &m)
{
    this->request.method = m;
}

void Request::setURI(const std::string &u)
{
    this->request.uri = u;
}

void Request::setHttpVersion(const std::string &v)
{
    this->request.httpVersion = v;
}

void Request::setBody(const std::string &b)
{
    this->request.body = b;
}

void Request::addHeader(const std::string &key, const std::string &value)
{
    this->request.headers[key] = value;
}

const std::string &Request::getMethod() const
{
    return this->request.method;
}

const std::string &Request::getURI() const
{
    return this->request.uri;
}

const std::string &Request::getHttpVersion() const
{
    return this->request.httpVersion;
}

const std::map<std::string, std::string> &Request::getHeaders() const
{
    return this->request.headers;
}

const std::string &Request::getBody() const
{
    return this->request.body;
}
