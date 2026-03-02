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

//                   key           value
// typedef std::map<std::string, std::string> HTTPHeader;

class Lexer
{
private:
    std::string content;
    std::vector<Token> tokens;
    unsigned int pos;
    unsigned int lpos;

    void increment();
    void decrement();
    unsigned int getPos() const;
    char &current();
public:
    Lexer();
    void tokenize(std::string &content);
    void setContent(std::string &content);

    
    void handleRequstline(std::string &buff);
    void handleHeaderline(std::string &buff, size_t &endofkey);
    void headerLineBufferFill(std::string &buff);
    void debug();
    std::string getTypeName(Token &token) const
    {
        switch (token.first)
        {
            case HEADER_NAME:         return "HEADER_NAME";
            case HEADER_VALUE:        return "HEADER_VALUE";
            case HEADER_REQUSET_LINE: return "HEADER_REQUSET_LINE";
            case METHOD:              return "METHOD";
            case URI:                 return "URI";
            case VERSION:             return "VERSION";
            default:                  return "UNKNOWN_TOKEN";
        }
    }
};