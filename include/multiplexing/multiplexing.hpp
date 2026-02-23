#pragma once

# include <iostream>
# include <string.h>
# include <errno.h>
# include <fcntl.h>
# include <unistd.h>
# include <sys/socket.h>
# include <sys/epoll.h>
# include <netinet/in.h>
# include <unistd.h>
# include <cstring>
# include <iostream>
# include <map>
/*  typedef context_t int(*ctx)(void);  */

int set_nonblocking(int sockfd);
typedef struct socket_context_s {
	int fd;
	int (*handler)(struct epoll_event);
} socket_context_t;
