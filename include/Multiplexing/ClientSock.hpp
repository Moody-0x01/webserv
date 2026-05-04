#pragma once
#include <Multiplexing/ServerSock.hpp>
#include <HTTP/Response.hpp>
#include <vector>

typedef struct Client: public ASocketContext {
public:
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
	void   unchunkify_buffer(void);

	void setip(std::string address);
	std::string getip(void);
	void setip_from_bytes(uint32_t ip_bytes);

	void setport_from_bytes(uint32_t ip_bytes);
	std::string getport(void);
	void setport(std::string port);

	std::vector<char> _buffer;
private:
	int			 _owner;
	std::string  ip;
	std::string  port;
} Client;
