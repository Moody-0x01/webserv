#include <Server.hpp>
#include <cstddef>
#include <map>
#include <string>
#include <unistd.h>
#include <fcntl.h>           /* Definition of AT_* constants */
#include <unistd.h>
#include <unistd.h>
#include <utility>
#include <vector>

std::vector<std::string> split(std::string str, char delim)
{
	std::vector<std::string> strings;
	std::string t;

	for (size_t i = 0; i < str.size(); ++i)
	{
		if (str[i] == delim)
        {
            if (!t.empty())
            {
                strings.push_back(t);
                t.clear();
            }
            while (i < str.size() && str[i] == delim)
                ++i;
            if (i < str.size())
                t += str[i];
			continue ;
        }
        t += str[i];
	}
	if (!t.empty())	strings.push_back(t);
	return (strings);
}

std::vector<std::string> split(std::string str, std::string delim)
{
	std::vector<std::string> strings;
	std::string t;

	for (size_t i = 0; i < str.size(); ++i)
	{
		if (delim.find(str[i]) != std::string::npos)
        {
            if (!t.empty())
            {
                strings.push_back(t);
                t.clear();
            }
            while (i < str.size() && delim.find(str[i]) != std::string::npos) ++i;
            if (i < str.size()) t += str[i];
			continue ;
        }
        t += str[i];
	}
	if (!t.empty())	strings.push_back(t);
	return (strings);
}

std::vector<std::string> split(std::vector<char> str, std::string delim)
{
	std::vector<std::string> strings;
	std::string t;

	for (size_t i = 0; i < str.size(); ++i)
	{
		if (delim.find(str[i]) != std::string::npos)
        {
            if (!t.empty())
            {
                strings.push_back(t);
                t.clear();
            }
            while (i < str.size() && delim.find(str[i]) != std::string::npos) ++i;
            if (i < str.size()) t += str[i];
			continue ;
        }
        t += str[i];
	}
	if (!t.empty())	strings.push_back(t);
	return (strings);
}

int set_nonblocking(int sockfd)
{
	errno = 0;
    int flags = fcntl(sockfd, F_GETFL);
    if (flags == -1) return -1;
	flags |= O_NONBLOCK;
    if (fcntl(sockfd, F_SETFL, flags) == -1) return -1;
    return 0;
}

std::string get_signal_name(int sig)
{
    static std::map<int, std::string> sig_map;

    sig_map[SIGINT]  =  "SIGINT (Interrupt)";
    sig_map[SIGTERM] =  "SIGTERM (Termination)";
    sig_map[SIGHUP]  =  "SIGHUP (Hangup/Reload)";
    sig_map[SIGUSR1] =  "SIGUSR1 (User Defined 1)";
    sig_map[SIGUSR2] =  "SIGUSR2 (User Defined 2)";
    sig_map[SIGQUIT] =  "SIGQUIT (Quit/Core Dump)";
    if (sig_map.count(sig)) return sig_map[sig];
    return "Unknown Signal";
}

void signal_handler(int sig)
{
	unsigned char* data;
    int saved_errno = errno;
	Multiplexer *self;

	self = Multiplexer::get_multiplexer(NULL);
	if (!self) throw "Well, failed to get a Multiplexer class";
	data = (unsigned char*)&sig;
    write(self->signal_io[1],
		data,
		sizeof(int));
    errno = saved_errno;
}

void sigpipe_handler(int sig)
{
	signal_handler(sig);	
	while (waitpid(-1, NULL, WNOHANG) > 0);
}

std::string serialize_headers(std::map<std::string, std::string> &headers, bool setdefault_status)
{
	std::string headers_as_str;
	std::string status_line, final;

	status_line = Response::status_lines[OK] + "\r\n";
	for (std::map<std::string, std::string>::iterator it = headers.begin(); it != headers.end(); ++it)
	{
		if (it->first == "status") {
			if (setdefault_status)
				status_line = "HTTP/1.0 " + it->second + "\r\n";
			continue ;
		}
		headers_as_str += it->first + ": " + it->second + "\r\n";
	}
	if (setdefault_status) {
		final = status_line + headers_as_str + "\r\n";
	} else 
		final = headers_as_str + "\r\n";
	return (final);
}

void close_fdlist(int fds[2])
{
	if (fds[STDIN_FILENO] != -1)  close(fds[STDIN_FILENO]);
	if (fds[STDOUT_FILENO] != -1) close(fds[STDOUT_FILENO]);
}


bool check_permissions(std::string file)
{
	return access(file.c_str(), F_OK | X_OK) != -1;
}

bool exists(std::string file)
{
	return access(file.c_str(), F_OK) != -1;
}

std::vector<char>::iterator search(std::vector<char> &vector, const char *pattern)
{
	return std::search(vector.begin(), vector.end(), pattern, pattern + std::strlen(pattern));
}

void print_buffer(std::vector<char> &buffer, const char *label)
{
	std::cout << label << " Size: " << buffer.size();
	size_t size;

	size = buffer.size();
	if (size < 60) {
		for (size_t i = 0; i < buffer.size(); i++)
			std::cout << buffer[i];
	} else {
		for (size_t i = 0; i < 30; i++)
			std::cout << buffer[i];
		std::cout << "\n.....\n";
		for (size_t i = size - 30; i < size; i++)
			std::cout << buffer[i];
	}
	std::cout << std::endl;
}

bool isheaders_valid(std::vector<std::string> &headers)
{
	for (size_t k = 0; k < headers.size(); k++)
	{
		if (headers[k].size() && headers[k][headers[k].size() - 1] == '\r')
			headers[k].erase(headers[k].size() - 1);
		if (!headers.size())
			return (false);
		for (size_t p = 0; p < headers[k].size() && headers[k][p] != ':'; p++)
		{
			if (headers[k][p] < 33 || headers[k][p] > 126)
                return (false);
		}
	}
	return (true);
}

bool epoll_switch(int epoll_fd, int fd, uint32_t mask, void *pointer)
{
	struct epoll_event event;

	event.events   = mask;
	event.data.ptr = pointer;
	if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fd, &event) < 0)
		return (false);
	return (true);
}

void push_into_buffer(std::vector<char> &buff, const char *src, ssize_t size)
{
	if (!size || !src)
		return ;
	buff.insert(buff.end(),
			src,
			src + size);
}

std::string collect(std::vector<char> &vector, size_t end)
{
	std::string result;

	for (size_t i = 0; (i < end) && (i < vector.size()); ++i)
		result.push_back(vector[i]);
	return (result);
}

const std::map<std::string, std::string>::iterator search(std::map<std::string, std::string> &map, std::string target)
{
	return (map.find(target));
}
