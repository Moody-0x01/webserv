#include <Server.hpp>

Lexer::Lexer() : content(""), pos(0) {}

void Lexer::tokenize(std::string &content)
{
    if (content.empty())
        return;
    this->setContent(content);

    std::string buff;
    unsigned int contentLen = this->content.length();

    while (getPos() < contentLen)
    {
        this->headerLineBufferFill(buff);

        if (buff.empty())
            break;

        size_t endofkey = buff.find(":");
        if (endofkey == std::string::npos)
            this->handleRequstline(buff);
        else
            this->handleHeaderline(buff, endofkey);
        buff.clear(); // flush the buffer
    }

    this->debug();
}

void Lexer::headerLineBufferFill(std::string &buff)
{
    while (getPos() < this->content.length())
    {
        char c = current();
        if (c == '\r')
        {
            increment(); // to skip the \r
            if (getPos() < this->content.length() && current() == '\n')
                increment(); // move past \n
            break;
        }
        buff += c;
        increment();
    }
}

void Lexer::handleHeaderline(std::string &buff, size_t &endofkey)
{
    std::string key = buff.substr(0, endofkey);
    if (!key.empty() && (key[key.length() - 1] == ' ' || key[key.length() - 1] == '\t'))
    {
        // catch to send a 400 Bad Request response.
        // dd("Invalid whitespace before colon in header");
    }
    endofkey++; // skip : of the key
    while (endofkey < buff.length() && (buff[endofkey] == ' ' || buff[endofkey] == '\t'))
        endofkey++; // skip white spaces and tabs
    std::string value = buff.substr(endofkey);
    tokens.push_back(Token(HEADER_NAME, key));
    tokens.push_back(Token(HEADER_VALUE, value));
}

void Lexer::handleRequstline(std::string &buff)
{
    size_t fspace = buff.find(' ');

    if (fspace == std::string::npos)
    {
        dd("no spaces on the request line? ??????");
        // throw BadRequestException();
    }
    else
    {
        std::string method = buff.substr(0, fspace);

        size_t sspace = buff.find(' ', fspace + 1);
        if (sspace == std::string::npos)
        {
            dd("missing HTTP version");
            // throw BadRequestException();
        }
        else
        {
            std::string uri = buff.substr(fspace + 1, sspace - (fspace + 1));
            std::string version = buff.substr(sspace + 1);
            tokens.push_back(Token(METHOD, method));
            tokens.push_back(Token(URI, uri));
            tokens.push_back(Token(VERSION, version));
        }
    }
}

void Lexer::setContent(std::string &content)
{
    this->content = content;
}

char &Lexer::current()
{
    return this->content[getPos()];
}

void Lexer::increment()
{
    this->pos++;
}

void Lexer::decrement()
{
    if (pos <= 0)
        return;
    this->pos--;
}

unsigned int Lexer::getPos() const
{
    return this->pos;
}

std::vector<Token> &Lexer::getTokens()
{
    return tokens;
}

// Debug
void Lexer::debug()
{
    for (size_t i = 0; i < tokens.size(); ++i)
    {
        std::string name = getTypeName(tokens.at(i));
        std::string val = tokens.at(i).second;
        dd("Token Type: [" + name + "] | Value: [" + val + "]");
    }
}
