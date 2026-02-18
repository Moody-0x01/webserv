# include <Server.hpp>
# include <cerrno>
# include <cstdio>
# include <iostream>
# include <map>
# include <sys/epoll.h>
# include <unistd.h>

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
	std::map<int, std::string> requests;

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
	event.events = EPOLLIN|EPOLLET;
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
        struct sockaddr_in client_addr;
		int ready = epoll_wait(epol_instance, events, EVENT_MAX, 5000);
		if (ready < 0)
		{
			std::cerr << "Epoll ctl failed\n";
			close(server_fd);
			return 1;
		}
		for (int index = 0; index < ready; ++index)
		{
			int ready_fd = events[index].data.fd;
			if (ready_fd == server_fd)
			{
				while (true)
				{
					socklen_t client_len = sizeof(client_addr);
					int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
					if (client_fd == -1) {
						if (errno == EAGAIN || errno == EWOULDBLOCK)
							break ; 
						std::cerr << "accept: " << strerror(errno) << "\n";
						break ;
					}
					set_nonblocking(client_fd);
					event.events = EPOLLIN|EPOLLET;
					event.data.fd = client_fd;
					if (epoll_ctl(epol_instance, EPOLL_CTL_ADD, client_fd, &event) == -1)
					{
						std::cerr << "epoll_ctl: " << strerror(errno) << "\n";
						close(client_fd);
					} else {
						requests[client_fd] = head;
						std::cout << "Client was added with fd=" << client_fd << "\n";
					}
				}
			} else {
				// Ready fd
				// Data from an existing client
				// Read data from the client
				if (events[index].events & EPOLLIN)
				{
					ssize_t count;
					char buffer[4096];
					while ((count = read(ready_fd, buffer, 4096)) > 0);
					std::cout << "[count ] " << count << "\n";
					if (count > 0)
						requests[ready_fd] += buffer;
					if (count == -1 && errno != EAGAIN) {
						std::cerr << "read: " << strerror(errno) << "\n";
						close(ready_fd);
					} else if (errno == EAGAIN || errno == EWOULDBLOCK)
					{
						event.events = EPOLLOUT|EPOLLET;
						event.data.fd = ready_fd;
						epoll_ctl(epol_instance, EPOLL_CTL_MOD, ready_fd, &event);
						std::cout << "Client done sending fd=" << ready_fd << "\n";
					}
				} else if (events[index].events & EPOLLOUT) {
					// generate response here...
					// Example: send back...
					const char *http_header = 
						"HTTP/1.0 200 OK\r\n"
						"Content-Type: text/html; charset=UTF-8\r\n"
						"Connection: close\r\n"
						"\r\n";
					requests[ready_fd] = http_header + requests[ready_fd] + tail;
					write(ready_fd, requests[ready_fd].c_str(), requests[ready_fd].size());
					close(ready_fd);
				}
			}
		}
    }
    close(server_fd);
    return 0;
}
