#include <Server.hpp>

namespace utility {

	void inject_cors(std::map<std::string, std::string> &h)
	{
		h["Allow"]                        = "GET, POST, DELETE, OPTIONS";
		h["Access-Control-Allow-Origin"]  = "*";
		h["Access-Control-Allow-Methods"] = "GET, POST, DELETE, OPTIONS";
		h["Access-Control-Allow-Headers"] = "*";
	}

	std::vector<std::string> split_by_two(const std::string &s, const std::string &delim)
	{
		std::vector<std::string> result;
		size_t pos = s.find(delim);
		if (pos == std::string::npos) {
			result.push_back(s);
			return result;
		}
		result.push_back(s.substr(0, pos));
		result.push_back(s.substr(pos + delim.size()));
		return result;
	}

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

	void signal_handler(int sig)
	{
		int saved_errno = errno;
		Multiplexer *self;

		self = Multiplexer::get_multiplexer(NULL);
		if (!self) throw "Well, failed to get a Multiplexer class";
		self->signal_handler.write_signal(sig);
		errno = saved_errno;
	}

	void sigpipe_handler(int sig)
	{
		signal_handler(sig);
		while (waitpid(-1, NULL, WNOHANG) > 0);
	}

	void logr(std::string &buffer)
	{
		std::cout << "|";
		for (size_t i = 0; i < buffer.size(); i++)
		{
			if (buffer[i] == '\r')
				std::cout << "\\r";
			else if (buffer[i] == '\n')
				std::cout << "\\n";
			else
				std::cout << buffer[i];
		}
		std::cout << "|\n";
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

	map_iterator search(const std::map<std::string, std::string> &map, std::string target)
	{
		return (map.find(target));
	}

	std::pair<bool, std::string> get_value(const std::map<std::string, std::string> &map, std::string target)
	{
		map_iterator ref = search(map, target);
		if (ref == map.end())
			return std::make_pair(false, "");
		return std::make_pair(true, ref->second);
	}

	std::string url_decode(const std::string &encoded)
	{
		std::string decoded;
		decoded.reserve(encoded.size());

		for (size_t i = 0; i < encoded.size(); ++i)
		{
			if (encoded[i] == '%' && i + 2 < encoded.size())
			{
				std::string hex = encoded.substr(i + 1, 2);
				char *end_ptr = NULL;
				long value = std::strtol(hex.c_str(), &end_ptr, 16);

				if (end_ptr == hex.c_str() + 2 && value >= 0 && value <= 255)
				{
					decoded += static_cast<char>(value);
					i += 2;
				}
				else
					decoded += encoded[i];
			}
			else if (encoded[i] == '+')
				decoded += encoded[i];
			else
				decoded += encoded[i];
		}

		return decoded;
	}
} // namespace utility
