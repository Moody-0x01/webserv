# pragma once
#include <algorithm>
#include <cstring>
# include <vector>
# include <map>
# include <string>

std::vector<std::string> split(std::string str, char delim);
std::vector<std::string> split(std::string str, std::string delim);
std::vector<std::string> split(std::vector<char> str, std::string delim);
std::string get_signal_name(int sig);
int set_nonblocking(int sockfd);
void signal_handler(int sig);
std::string serialize_headers(std::map<std::string, std::string> headers, bool setdefault_status);
void close_fdlist(int fds[2]);
bool check_permissions(std::string file);
bool exists(std::string file);
std::vector<char>::iterator search(std::vector<char> &vector, const char *pattern);
void print_buffer(std::vector<char> &buffer, const char *label);
