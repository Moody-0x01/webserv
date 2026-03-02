#include <Server.hpp>

HttpParser::HttpParser() : currentState(IDLE), lexerInstence(), parent(NULL)
{
}

void HttpParser::handle()
{
    if (!this->validated())
        return;
    if (parent->request_buffer.find("\r\n\r\n") != std::string::npos)
    {
        // we got all HEaders triggeing the lexer
        this->currentState = READY;
        this->lexerInstence.tokenize(getRequestBuffer());
    }
}

std::string &HttpParser::getRequestBuffer()
{
    return parent->request_buffer;
}

std::string &HttpParser::getResponseBuffer()
{
    return parent->response_buffer;
}

ParserState HttpParser::state() const
{
    return currentState;
}

void HttpParser::setParent(SocketContext *client)
{
    this->parent = client;
}

SocketContext *HttpParser::getParent() const
{
    return this->parent;
}

bool HttpParser::validated()
{
    bool validated = false;

    validated = (parent != NULL);
    // more validation shit here ...

    return validated;
}