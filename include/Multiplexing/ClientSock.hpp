#pragma once
#include <Multiplexing/ServerSock.hpp>
#include <HTTP/Response.hpp>
#include <cstddef>
#include <vector>

# define CLIENT_TIMEOUT 30

typedef struct Client: public ASocketContext {
public:
	Client();
	~Client();
	void action(uint32_t e) __THROWS_STRERROR;
	HttpParser parserInstance;
	Response   response;
	void free();
	HttpParser &getParser();

	Server *get_server(void) const __THROWS_STRERROR;
	void   parse_request(void)  __THROWS_STRERROR;
	void   generate_response(void) __THROWS_STRERROR;
	void   switch_mode(socket_mode_t mode) __THROWS_STRERROR;
	void   read_into_request_buffer(void) __THROWS_STRERROR;
	void   set_owner(int owner);
	int    get_owner(void) const;
	bool   unchunkify_buffer(void);
	bool   push_into_client_buffer(const char buff[READ_CHUNK_SIZE], ssize_t count);

	void setip(std::string address);
	std::string getip(void);
	void setip_from_bytes(uint32_t ip_bytes);

	void setport_from_bytes(uint32_t ip_bytes);
	std::string getport(void);
	void setport(std::string port);
	bool timeout(void) __THROWS_STRERROR;

	std::vector<char> _buffer;
    time_t            last_event_time;   // Bytes left to read in the CURRENT chunk 
private:
	int			 _owner;
	std::string  ip;
	std::string  port;
} Client;
