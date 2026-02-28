#include "../../../include/Parser/HTTP/Lexer.hpp"

Lexer::Lexer() : pos(0), lpos(0)
{
    loadReservedKeys();
    tokenize();
}

void Lexer::tokenize()
{
    if (content.empty()) return;
    
}

bool Lexer::isReserved(std::string key) const
{
    if (key.length() == 0) return false;

    if (std::find(reservedKeys.begin(), reservedKeys.end(), key) != reservedKeys.end())
        return true;
    return false;
}

void Lexer::loadReservedKeys()
{
    reservedKeys.push_back("GET");
    reservedKeys.push_back("Host:");
    reservedKeys.push_back("User-Agent:");
    reservedKeys.push_back("Accept-Encoding:");
    reservedKeys.push_back("Accept-Language:");
    reservedKeys.push_back("Accept-Charset:");
    reservedKeys.push_back("Accept:");
}
