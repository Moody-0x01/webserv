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
    METHOD,
    HOST,
    CONTENT_TYPE,
    CONTENT_LENGTH,
    SPACE,
    PATH,
    HTTP_VERSION,
    CRLF,
    KEY,
    VALUE,
};

typedef std::map<std::string, std::string> HTTPHeader;
typedef std::pair<HttpTokenType, std::string> Token;

class Lexer
{
private:
    std::vector<std::string> content;
    std::vector<Token> tokens;
    Token currentToken;
    Token prevToken;
    unsigned int pos;
    unsigned int lpos;
    std::vector<std::string> reservedKeys;
    std::vector<std::vector<std::string> > sContent;

public:
    Lexer();
    // read the content (i will use file for now and store it on content string)
    void readStream(std::istream &input);
    void tokenize();
    void loadReservedKeys();
    bool isReserved(std::string key) const;
};