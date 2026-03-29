#pragma once

#include <iostream>
#include <Parser/HTTP/Lexer.hpp>
#include <HTTP/Request.hpp>
#include <HTTP/Response.hpp>
#include <algorithm>

// 
//  METHOD,
// HOST,
// CONTENT_TYPE,
// CONTENT_LENGTH,
// SPACE,
// PATH,
// HTTP_VERSION,
// CRLF,
// KEY,
// VALUE,

enum ParserState
{
    REQUEST_LINE,
    HEADERS_DONE,
    BODY,
    IDLE,
    READY,
};

// For debugging only
template<typename T>
void dd(const T &s);

struct SocketContext;

class HttpParser
{
private:
    ParserState currentState;
    Lexer lexerInstence;
    SocketContext *parent;
    Request request;

    unsigned int targetBodySize;

public:
    HttpParser();

    void handle();

    std::string &getRequestBuffer();
    std::string &getResponseBuffer();
    Request &getRequestObject();
    ParserState state() const;

    SocketContext *getParent() const;
    void setParent(SocketContext *client);
    bool isHeaderValueExist(const std::string &key);
    bool validated();
    bool requestValidation();
};
