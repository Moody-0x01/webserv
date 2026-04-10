#pragma once
#include <Multiplexing/ServerSock.hpp>
#include <HTTP/Response.hpp>

typedef struct Client: public ASocketContext {
public:
	void action(uint32_t e) __THROWS_STRERROR;
	void take_ownership(ASocketContext *Other);
	HttpParser parserInstance;
	Response   response;
	void free();
	HttpParser &getParser();

	Server *get_server(void) const __THROWS_STRERROR;
	void parse_request(void)  __THROWS_STRERROR;
	void generate_response(void) __THROWS_STRERROR;
	void set_owner(int owner);
	int  get_owner(void) const;
private:
	int  _owner;
} Client;
