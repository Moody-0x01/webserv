#pragma once

#include <string>
#include <map>

#define  READ_CHUNK_SIZE      4096
#define  WRITE_CHUNK_SIZE     READ_CHUNK_SIZE // 4kb each time.
#define  NO_CODE 999

typedef struct HttpRequest {
	int owner, conn;
	unsigned short code;
	bool isbadrequest;
    std::string method;
    std::string uri;
    std::string httpVersion;
    std::map<std::string, std::string> headers;
    std::string body;
    std::map<std::string, std::string> params;
    std::string query_string;
} HttpRequest;

# define __THROWS_STRERROR throw(const char *)
class Request
{
private:
	HttpRequest request;
public:
    Request(/* args */);
    ~Request();

    void setMethod(const std::string &m);
    void setURI(const std::string &u);
    void setHttpVersion(const std::string &v);
    void addHeader(const std::string &key, const std::string &value);
    void setBody(const std::string &b);

    const std::string &getMethod() const;
    const std::string &getURI() const;
    const std::string &getHttpVersion() const;
    const std::map<std::string, std::string> &getHeaders() const;
    const std::string &getBody() const;
	// TODO: This function sets up who are the server and client that are responsible for this current request aka owner and conn
	void set_sockets(const int server, const int client);
	const HttpRequest &getHttpRequest(void) const;
	HttpRequest &getHttpRequest(void);
	void setcode(int code);
};
