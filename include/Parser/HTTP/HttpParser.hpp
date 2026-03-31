#pragma once

#include <iostream>
#include <Parser/HTTP/Lexer.hpp>
#include <HTTP/Request.hpp>
#include <HTTP/Response.hpp>
#include <algorithm>
#include <string>
#include <vector>

enum ParserState
{
    REQUEST_LINE,
    HEADERS_DONE,
    BODY,
    IDLE,
    READY,
};

typedef std::pair<std::string, std::string> Param;

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
    void parseParams(std::string  &uri);
    std::string &getRequestBuffer();
    std::string &getResponseBuffer();
    Request &getRequestObject();
    ParserState state() const;

    SocketContext *getParent() const;
    void setParent(SocketContext *client);
    bool isHeaderValueExist(const std::string &key);
};
