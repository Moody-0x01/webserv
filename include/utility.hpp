# pragma once
# include <vector>
# include <string>

std::vector<std::string> split(std::string str, char delim);
std::vector<std::string> split(std::string str, std::string delim);
std::string get_signal_name(int sig);
int set_nonblocking(int sockfd);
void signal_handler(int sig);
