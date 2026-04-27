#pragma once

#include <iostream>
#include <Parser/HTTP/Lexer.hpp>
#include <HTTP/Request.hpp>
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

struct Client;

class HttpParser
{
private:
    ParserState currentState;
    Lexer lexerInstence;
    Client *parent;
    Request request;
    long targetBodySize;
public:
    HttpParser();
    void handle();
    void parseParams(std::string  &uri);
    Request &getRequestObject();
    ParserState state() const;
	void  setChunkedEncoding(void);
	bool  parseContentLength(void);
    Client *getParent() const;
    void setParent(Client *client);
    bool isHeaderValueExist(const std::string &key);
	const std::map<std::string, std::string>::iterator getHeaderValue(std::string header_key);
};
