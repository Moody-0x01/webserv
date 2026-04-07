#pragma once

#include <iostream>
#include <map>
#include <string>
#include <fstream>
#include <vector>
#include <sstream>
#include <algorithm>

enum HttpTokenType
{
    HEADER_NAME,
    HEADER_VALUE,
    HEADER_REQUSET_LINE,
    METHOD,
    URI,
    VERSION
};

typedef std::pair<HttpTokenType, std::string> Token;

class Lexer
{
private:
    std::string content;
    std::vector<Token> tokens;
    unsigned int pos;
    void increment();
    void decrement();
    char &current();
    unsigned int getPos() const;
    bool badRequest;
    void markAsBad();
public:
    Lexer();
    void tokenize(std::string &content);
    void setContent(std::string &content);

    void handleRequstline(std::string &buff);
    void handleHeaderline(std::string &buff, size_t &endofkey);
    void headerLineBufferFill(std::string &buff);
    std::vector<Token> &getTokens();
    bool isBadRequest() const;
};
