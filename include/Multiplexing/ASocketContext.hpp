#pragma once
# include <string>
# include <sys/epoll.h>
# include <sys/socket.h>
# include <sys/types.h>
# include <unistd.h>
# include <cassert>
# include <netinet/in.h>
# include <Parser/HTTP/HttpParser.hpp>
# include <Parser/Config/Config.hpp>
# include <HTTP/Request.hpp>

# define __THROWS_STRERROR throw(const char *)
typedef struct ASocketContext
{
public:
	ASocketContext(int sock);
	ASocketContext();
	virtual ~ASocketContext();


	std::string request_buffer;
	std::string response_buffer;
	void virtual action(uint32_t e) __THROWS_STRERROR = 0;

	void set_socket(int sockfd);
	int get_socket(void) const;
	void disown(void);
private:
	int  _sockfd;
	bool _owns_fd;
} ASocketContext;
