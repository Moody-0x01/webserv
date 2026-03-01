#include <Server.hpp>

HttpParser::HttpParser() : parent(NULL)
{
    lexerInstence = Lexer(this);
}

void HttpParser::handle(ssize_t count)
{
    if (!this->validated())
        return;
    std::string safe_buffer(buffer, count);

    std::cout << "RECEIVED:\n"<< safe_buffer << std::endl;

    std::cout << "PARENT BUFFER:\n" << parent->request_buffer << std::endl;
    // testing only
    if (parent->request_buffer.find("\r\n\r\n") != std::string::npos)
    {
        this->currentState = READY;
    }
}

char *HttpParser::getBuffer()
{
    return buffer;
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