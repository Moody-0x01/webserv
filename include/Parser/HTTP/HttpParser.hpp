#pragma once

#include <iostream>
#include <Parser/HTTP/Lexer.hpp>

enum ParserState
{
    REQUEST_LINE,
    HEADERS,
    BODY,
    IDLE,
    READY,
};

#define BUFFER_SIZE 4096

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

    void handle(ssize_t count);

   char* getBuffer();

   ParserState state() const;

   SocketContext *getParent() const;
   void setParent(SocketContext *client);

   bool validated();
};