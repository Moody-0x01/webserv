#pragma once
#include <Multiplexing/ServerSock.hpp>
#include <HTTP/Response.hpp>

typedef struct Client: public ASocketContext {
public:
	~Client();
	void action(uint32_t e) __THROWS_STRERROR;
	HttpParser parserInstance;
	Response   response;
	void free();
	HttpParser &getParser();

	Server *get_server(void) const __THROWS_STRERROR;
	void parse_request(void)  __THROWS_STRERROR;
	void generate_response(void) __THROWS_STRERROR;
	void set_owner(int owner);
	int  get_owner(void) const;

	void setip(std::string address);
	void setip_from_bytes(uint32_t ip_bytes);
	std::string getip(void);


private:
	int			 _owner;
	std::string  ip;
} Client;
