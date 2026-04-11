#include <Server.hpp>

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
