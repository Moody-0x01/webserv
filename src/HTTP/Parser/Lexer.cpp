#include <Server.hpp>

Lexer::Lexer() : content(""), pos(0), lpos(0) {}

void Lexer::tokenize(std::string &content)
{
    this->setContent(content);

    if (content.empty())
        return;

    std::string buff;
    unsigned int contentLen = content.length();
    while (getPos() < contentLen)
    {
        while (getPos() < contentLen)
        {
            char c = current();
            if (c == '\r') // i read full line to the end and token each one alone
            {
                increment(); // to skip the new line
                if (getPos() < contentLen && current() == '\n')
                    increment(); // move past \n
                break;
            }
            buff += c;
            increment();
        }

        if (buff.empty()) {
            break; 
        }

        size_t endofkey = buff.find(":");
        if (endofkey == std::string::npos)
        {
            // GET /index.html HTTP/1.1 
            size_t fspace = buff.find(' ');

            if (fspace == std::string::npos)
            {
                // dd("no spaces");
                // throw BadRequestException();
            } else {
                std::string method = buff.substr(0, fspace);

                size_t sspace = buff.find(' ', fspace + 1);
                if (sspace == std::string::npos) 
                {
                    // dd("missing HTTP version");
                    // throw BadRequestException();
                } 
                else 
                {
                    std::string uri = buff.substr(fspace + 1, sspace - (fspace + 1));
                    std::string version = buff.substr(sspace + 1);
                    tokens.push_back(Token(METHOD, method));
                    tokens.push_back(Token(URI, uri));
                    tokens.push_back(Token(VERSION, version));
                    
                    dd("Parsed Request Line -> Method: [" + method + "] URI: [" + uri + "] Version: [" + version + "]");
                    dd("---------------------------------------");
                }
            }
        }
        else
        {
            std::string key = buff.substr(0, endofkey);
            if (!key.empty() && (key[key.length() - 1] == ' ' || key[key.length() - 1] == '\t'))
            {
                // catch to send a 400 Bad Request response.
                // dd("CRITICAL ERROR: Invalid whitespace before colon in header!");
            }
            endofkey++; // skip : of the key
            while (endofkey < buff.length() && (buff[endofkey] == ' ' || buff[endofkey] == '\t'))
                endofkey++; // skip white spaces and tabs
            std::string value = buff.substr(endofkey);
            tokens.push_back(Token(HEADER_NAME, key));
            tokens.push_back(Token(HEADER_VALUE, value));
        }
        buff.clear(); // flush the buffer
    }

    for (size_t i = 0; i < tokens.size(); ++i)
    {
        std::string name = getTypeName(tokens.at(i));
        std::string val = tokens.at(i).second;
        dd("Token Type: [" + name + "] | Value: [" + val + "]");
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

// Debug
template <typename T>
void dd(const T &s)
{
    std::cout << s << std::endl;
}
