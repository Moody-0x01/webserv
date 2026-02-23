# include <cstdlib>
# include <multiplexing/multiplexing.h>

/*  const char *head = "<!DOCTYPE html>\n"  */
/*  	"<html lang=\"en\">\n"  */
/*  	"<head>\n"  */
/*  	"<meta charset=\"UTF-8\">\n"  */
/*      "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n"  */
/*      "<title>Styled Paragraph</title>\n"  */
/*      "<style>\n"  */
/*  	"body {\n"  */
/*             " margin: 0;\n"  */
/*             " padding: 20px;\n"  */
/*             " background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);\n"  */
/*             " min-height: 100vh;\n"  */
/*             " display: flex;\n"  */
/*             " justify-content: center;\n"  */
/*             " align-items: center;\n"  */
/*             " font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;\n"  */
/*         " }\n"  */
/*         " p {\n"  */
/*             " max-width: 600px;\n"  */
/*             " background: white;\n"  */
/*             " padding: 40px;\n"  */
/*             " border-radius: 12px;\n"  */
/*             " box-shadow: 0 20px 60px rgba(0, 0, 0, 0.3);\n"  */
/*             " line-height: 1.8;\n"  */
/*             " font-size: 18px;\n"  */
/*             " color: #333;\n"  */
/*             " letter-spacing: 0.5px;\n"  */
/*             " border-left: 5px solid #667eea;\n"  */
/*             " transition: transform 0.3s ease, box-shadow 0.3s ease;\n"  */
/*         " }\n"  */
/*         " p:hover {\n"  */
/*             " transform: translateY(-5px);\n"  */
/*             " box-shadow: 0 25px 70px rgba(0, 0, 0, 0.35);\n"  */
/*         " }\n"  */
/*  	   " </style>\n</head>\n<body>\n<p>\n";  */
/**/
/*  const char *tail = "</p>\n"  */
/*  "</body>\n"  */
/*  "</html>\n";  */
/**/
/**/
/*  std::map<int, std::string> requests;  */
/*  int accept_handler(struct epoll_event *ev)  */
/*  {  */
/*  	struct sockaddr addr;  */
/*  	socklen_t len = sizeof(addr);  */
/*  	int conn = accept(client, &addr, &len);  */
/*  	if (conn == -1)  */
/*  	{  */
/*  		std::cerr << "accept: " << strerror(errno) << "\n";  */
/*  		continue ;  */
/*  	}  */
/*  	// Register new conn with epoll in ET mode  */
/*  	struct epoll_event cev;  */
/*  	cev.events = EPOLLIN;  */
/*  	cev.data.fd = conn;  */
/*  	set_nonblocking(conn);  */
/*  	if (epoll_ctl(epol_instance, EPOLL_CTL_ADD, conn, &cev) == -1) {  */
/*  		std::cerr << "epoll_ctl: " << strerror(errno) << "\n";  */
/*  		return (EXIT_FAILURE);  */
/*  	}  */
/*  	requests[conn] = head;  */
/*  	return (EXIT_SUCCESS);  */
/*  }  */
/**/
/*  int client_handler(struct epoll_event *ev)  */
/*  {  */
/*  	// TODO: Make a handler to read the request.  */
/*  	// TODO: also it has to handle when a client does not have anything else to send so it gotta just.  */
/*  	// reset the event ev.  */
/*  	return (0);  */
/*  }  */
/**/
int set_nonblocking(int sockfd) {
	errno = 0;
    int flags = fcntl(sockfd, F_GETFL, 0);
    if (flags == -1) {
		std::cerr << "set_nonblocking: " << strerror(errno);
        return -1;
    }
    if (fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) == -1) {
		std::cerr << "set_nonblocking: " << strerror(errno);
        return -1;
    }
    return 0;
}
