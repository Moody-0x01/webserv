# pragma once
# include <algorithm>
# include <cstring>
# include <vector>
# include <map>
# include <string>
# include <stdint.h>

typedef const std::map<std::string, std::string>::const_iterator map_iterator;

std::vector<std::string> split(std::string str, char delim);
std::vector<std::string> split(std::string str, std::string delim);
std::vector<std::string> split(std::vector<char> str, std::string delim);

std::string url_decode(const std::string &encoded);

int set_nonblocking(int sockfd);
void signal_handler(int sig);
void sigpipe_handler(int sig);
std::string serialize_headers(std::map<std::string, std::string> &headers, bool setdefault_status);
void close_fdlist(int fds[2]);
bool check_permissions(std::string file);
bool exists(std::string file);
void print_buffer(std::vector<char> &buffer, const char *label);
bool isheaders_valid(std::vector<std::string> &headers);
bool epoll_switch(int epoll_fd, int fd, uint32_t mask, void *pointer);
void push_into_buffer(std::vector<char> &buff, const char *src, ssize_t size);
std::string collect(std::vector<char> &vector, size_t end);
std::vector<char>::iterator search(std::vector<char> &vector, const char *pattern);
map_iterator search(const std::map<std::string, std::string> &map, std::string target);
std::pair<bool, std::string> get_value(const std::map<std::string, std::string> &map, std::string target);
