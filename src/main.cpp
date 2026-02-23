# include <Server.hpp>
# include <cerrno>
# include <cstdio>
# include <iostream>
# include <sys/epoll.h>
# include <sys/socket.h>
# include <unistd.h>

std::map<int, std::string> requests;
const char *head = "<!DOCTYPE html>\n"
	"<html lang=\"en\">\n"
	"<head>\n"
	"<meta charset=\"UTF-8\">\n"
    "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n"
    "<title>Styled Paragraph</title>\n"
    "<style>\n"
	"body {\n"
           " margin: 0;\n"
           " padding: 20px;\n"
           " background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);\n"
           " min-height: 100vh;\n"
           " display: flex;\n"
           " justify-content: center;\n"
           " align-items: center;\n"
           " font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;\n"
       " }\n"
       " p {\n"
           " max-width: 600px;\n"
           " background: white;\n"
           " padding: 40px;\n"
           " border-radius: 12px;\n"
           " box-shadow: 0 20px 60px rgba(0, 0, 0, 0.3);\n"
           " line-height: 1.8;\n"
           " font-size: 18px;\n"
           " color: #333;\n"
           " letter-spacing: 0.5px;\n"
           " border-left: 5px solid #667eea;\n"
           " transition: transform 0.3s ease, box-shadow 0.3s ease;\n"
       " }\n"
       " p:hover {\n"
           " transform: translateY(-5px);\n"
           " box-shadow: 0 25px 70px rgba(0, 0, 0, 0.35);\n"
       " }\n"
	   " </style>\n</head>\n<body>\n<p>\n";

const char *tail = "</p>\n"
"</body>\n"
"</html>\n";
# define EVENT_MAX 4096
int main() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);

	struct epoll_event event;
	struct epoll_event events[EVENT_MAX];
    if (server_fd < 0) {
        std::cerr << "Socket creation failed\n";
        return 1;
    }
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    struct sockaddr_in address;
    std::memset(&address, 0, sizeof(address));
    std::memset(&event, 0, sizeof(event));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8080);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        std::cerr << "Bind failed\n";
        close(server_fd);
        return 1;
    }
	if (set_nonblocking(server_fd) == -1) return 1;

    if (listen(server_fd, 3) < 0) {
        std::cerr << "Listen failed\n";
        close(server_fd);
        return 1;
    }

	int epol_instance = epoll_create(1024);
	if (epol_instance < 0)
	{
        std::cerr << "Epoll failed\n";
        close(server_fd);
        return 1;
	}
    std::cout << "HTTP/1.0 Server listening on localhost:8080\n";
	event.events = EPOLLIN;
    event.data.fd = server_fd;
	int code = epoll_ctl(epol_instance, EPOLL_CTL_ADD, server_fd, &event);
	if (code < 0)
	{
        std::cerr << "Epoll ctl failed\n";
        close(server_fd);
        return 1;
	}
    while (true)
	{
		int ready = epoll_wait(epol_instance, events, EVENT_MAX, 5000);
		if (ready < 0)
		{
			std::cerr << "epoll_wait: " << strerror(errno) << "\n";
			close(server_fd);
			return 1;
		}
		for (int index = 0; index < ready; ++index)
		{
			int client = events[index].data.fd;
			int event_mask = events[index].events;
			if (client == server_fd)
			{
				struct sockaddr addr;
				socklen_t len = sizeof(addr);
				int conn = accept(client, &addr, &len);
				if (conn == -1)
				{
					std::cerr << "accept: " << strerror(errno) << "\n";
					continue ;
				}
				// Register new conn with epoll in ET mode
				struct epoll_event cev;
				cev.events = EPOLLIN;
				cev.data.fd = conn;
				set_nonblocking(conn);
				if (epoll_ctl(epol_instance, EPOLL_CTL_ADD, conn, &cev) == -1) {
					std::cerr << "epoll_ctl: " << strerror(errno) << "\n";
					continue ;
				}
				requests[conn] = head;
			}
			if (event_mask & EPOLLIN) {
				// Ready for a single read?.
				std::cout << "Reading >~<\n";
				char buff[4096];
				ssize_t count = read(client, buff, sizeof(buff));
				if (count == 0) {
					struct epoll_event cev;
					cev.events |= EPOLLOUT;
					cev.data.fd = client;
					if (epoll_ctl(epol_instance, EPOLL_CTL_MOD, client, &cev) == -1) {
						std::cerr << "epoll_ctl: " << strerror(errno) << "\n";
						continue ;
					}
					requests[client] += tail;
				} else if (count == -1) {
					// reading encounterred an error and probably needs to stop and remove fd from epoll?
					std::cerr << "read: " << strerror(errno) << "\n";
					close(client);
					if (epoll_ctl(epol_instance, EPOLL_CTL_DEL, client, NULL) == -1) {
						std::cerr << "epoll_ctl: " << strerror(errno) << "\n";
						continue ;
					}
				}
				requests[client] += buff;
			} else if (event_mask & EPOLLOUT) {
				// Ready for a single write?.
				std::cout << "Writing >~<\n";
				ssize_t count = write(client, requests[client].c_str(), requests[client].size());
				if (count == -1)
					std::cerr << "write: " << strerror(errno) << "\n";
				close(client);
				if (epoll_ctl(epol_instance, EPOLL_CTL_DEL, client, NULL) == -1) {
					std::cerr << "epoll_ctl: " << strerror(errno) << "\n";
					continue ;
				}
			}
		}
    }
    close(server_fd);
    return 0;
}
