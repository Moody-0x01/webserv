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
    NEWLINE
};

typedef std::pair<HttpTokenType, std::string> Token;

//                   key           value
typedef std::map<std::string, std::string> HTTPHeader;

class Lexer
{
private:
    std::string content;
    std::vector<Token> tokens;
    unsigned int pos;
    unsigned int lpos;

public:
    Lexer();
    void tokenize(std::string &content);
    void setContent(std::string &content);
};