#pragma once
#include <Multiplexing/ASocketContext.hpp>

typedef struct Server: public ASocketContext {
public:
	void action(uint32_t e) __THROWS_STRERROR;
} Server;
