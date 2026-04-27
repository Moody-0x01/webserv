#pragma once

#include <string>
#include <map>
#include <sys/types.h>
#include <vector>

#define  READ_CHUNK_SIZE      1024 * 64
#define  WRITE_CHUNK_SIZE     READ_CHUNK_SIZE
#define  NO_CODE 999

typedef struct HttpRequest {
	int owner, conn;
	unsigned short code;
	bool isbadrequest, ischunked;
    std::string method;
    std::string uri;
    std::string httpVersion;
    std::map<std::string, std::string> headers;
    std::map<std::string, std::string> params;
	std::vector<char> *body;
    std::string query_string;
	ssize_t content_length;
} HttpRequest;

# define __THROWS_STRERROR throw(const char *)
class Request
{
private:
	HttpRequest request;
public:
    Request();
    ~Request();


    void setMethod(const std::string &m);
    void setURI(const std::string &u);
    void setHttpVersion(const std::string &v);
    void addHeader(const std::string &key, const std::string &value);
	void setBody(std::vector<char> *b);

    const std::string &getMethod() const;
    const std::string &getURI() const;
    const std::string &getHttpVersion() const;
    const std::map<std::string, std::string> &getHeaders() const;
    const std::vector<char> *getBody() const;
	// TODO: This function sets up who are the server and client that are responsible for this current request aka owner and conn
	void set_sockets(const int server, const int client);
	const HttpRequest &getHttpRequest(void) const;
	HttpRequest &getHttpRequest(void);
	void setcode(int code);
};
