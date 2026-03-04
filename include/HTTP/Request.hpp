#pragma once

#include <string>
#include <map>

class Request
{
private:
    std::string method;
    std::string uri;
    std::string httpVersion;
    std::map<std::string, std::string> headers;
    std::string body;

    unsigned int code;
public:
    Request(/* args */);
    ~Request();

    void setMethod(const std::string &m);
    void setURI(const std::string &u);
    void setHttpVersion(const std::string &v);
    void addHeader(const std::string &key, const std::string &value);
    void setBody(const std::string &b);
    void setCode(const unsigned int code);

    const std::string &getMethod() const;
    const std::string &getURI() const;
    const std::string &getHttpVersion() const;
    const std::map<std::string, std::string> &getHeaders() const;
    const std::string &getBody() const;
};
