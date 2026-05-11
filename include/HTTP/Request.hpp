#pragma once

#include <stdint.h>
#include <stdint.h>
#include <sstream>
#include <string>
#include <map>
#include <sys/types.h>
#include <vector>

typedef const std::map<std::string, std::string>::const_iterator map_iterator;
# define  READ_CHUNK_SIZE      1024 * 64
# define  SENDING_CHUNK_SIZE   1024 * 16
# define  WRITE_CHUNK_SIZE     READ_CHUNK_SIZE
# define  MAX_HEADER_SIZE      16 * 1024
# define  NO_CODE 999
# define CR '\r'
# define LF '\n'
// # define CRLF "\r\n"
typedef uint8_t ChunkStatusBit;

# define CHUNK_START     1  <<  0  //  0000 | 0001
# define CHUNK_SIZE      1  <<  1  //  0000 | 0010
# define CHUNK_DATA      1  <<  2  //  0000 | 0100
# define CHUNK_TRAILER   1  <<  3  //  0000 | 1000
# define CHUNK_COMPLETE  1  <<  4  //  0001 | 0000
# define CHUNK_ERROR     1  <<  5  //  0010 | 0000
// # define CHUNK_EXT       1  <<  6  //  0100 | 0000

typedef struct ChunkContext {
	std::string       hex;
    ChunkStatusBit    status_mask;
    unsigned long     remaining;
	unsigned long     cursor;
	char              prev;
	std::vector<char> chunk_data;

	void convert_remaining_into_hex(void);

	bool is_reading_size(void);
	bool is_reading_data(void);
	bool is_done(void);
	bool is_reading_crlf(void);

	void consume_chunk_data(std::vector<char> &buffer);
	void consume_chunk_size(std::vector<char> &buffer);
	void skip_crlf(std::vector<char> &buffer);

	void strip_trailer(std::vector<char> &data, size_t offset);
	void unpack(std::vector<char> &buffer);
	bool just_started(void);
	void log_buffer(std::vector<char> &buffer);
	bool is_corrupted(void);

    ChunkContext();
} ChunkContext;

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
	ChunkContext chunked_context;
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
	std::string getMethod();
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
