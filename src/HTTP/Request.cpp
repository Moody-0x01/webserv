#include <Server.hpp>

Request::Request() {};

Request::~Request() {};

void Request::setMethod(const std::string &m)
{
    this->method = m;
}

void Request::setURI(const std::string &u)
{
    this->uri = u;
}

void Request::setHttpVersion(const std::string &v)
{
    this->httpVersion = v;
}

voi Request::setBody(const std::string &b)
{
    this->body = b;
}

void Request::addHeader(const std::string &key, const std::string &value)
{
    this->headers[key] = value;
}

void Request::setCode(const unsigned int code)
{
    this->code = code;
}

const std::string &Request::getMethod() const
{
    return this->method;
}

const std::string &Request::getURI() const
{
    return this->uri;
}

const std::string &Request::getHttpVersion() const
{
    return this->httpVersion;
}

const std::map<std::string, std::string> &Request::getHeaders() const
{
    return this->headers;
}

const std::string &Request::getBody() const
{
    return this->body;
}
