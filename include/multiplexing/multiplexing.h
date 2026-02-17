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

int set_nonblocking(int sockfd);
