# pragma once
#include <Multiplexing/SocketContext.hpp>

void client_handler(uint32_t e, Client *Self); // NOTE: Request/Response, client events, disconnection. and so on
											   // In other words, the parsing and generation of the response wsf
void server_handler(uint32_t e, Server *Self); // NOTE: Handling the incommming connections from here, register clients..
