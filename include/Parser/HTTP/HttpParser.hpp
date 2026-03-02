#pragma once

#include <iostream>
#include <Parser/HTTP/Lexer.hpp>

#define BUFFER_SIZE 4096

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
    HEADERS,
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
    char buffer[BUFFER_SIZE];
    SocketContext *parent;

public:
    HttpParser();

    void handle();

    std::string &getRequestBuffer();
    std::string &getResponseBuffer();

    ParserState state() const;

    SocketContext *getParent() const;
    void setParent(SocketContext *client);

    bool validated();
};