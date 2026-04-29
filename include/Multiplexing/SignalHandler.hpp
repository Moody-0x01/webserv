#pragma once
#include <Multiplexing/ASocketContext.hpp>

typedef struct SignalHandler: public ASocketContext {
private:
	int io[2];
public:
	void init(void) __THROWS_STRERROR;
	void epoll_register(int epoll_fd) __THROWS_STRERROR;
	void action(uint32_t e) __THROWS_STRERROR;
	void set_signalio(int io[2]);
	void setup_signal_handlers(void) __THROWS_STRERROR;
	void write_signal(int signum) __THROWS_STRERROR;
	int read_signal(void) __THROWS_STRERROR;
	~SignalHandler(void);
} SignalHandler;
