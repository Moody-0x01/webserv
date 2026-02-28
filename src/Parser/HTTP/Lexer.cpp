#include "../../../include/Parser/HTTP/Lexer.hpp"

Lexer::Lexer() : pos(0), lpos(0)
{
    loadReservedKeys();
    std::ifstream input("./src/Parser/HTTP/get");
    readStream(input);
    tokenize();
}

void Lexer::readStream(std::istream& input)
{
    std::string line;
    while (std::getline(input, line))
        content.push_back(line);
}

void Lexer::tokenize()
{
    if (content.empty()) return;
    pos = 0;
    lpos = 0;
    for (size_t i = 0; i < content.size(); i++)
    {
        std::string line = content.at(i);
        if (line.length() == 0) break;
        std::istringstream stream(line);
        std::vector<std::string> parts;
        while (stream >> line)
        {
            parts.push_back(line);
        }
        sContent.push_back(parts);
        lpos++;
    }

    for (size_t i = 0; i < sContent.size(); i++)
    {
        for (size_t j = 0; j < sContent[i].size(); j++)
        {
            std::string string = sContent[i][j];
            if (isReserved(string) && j == 0)
            {
                // std::cout << "Key " << string << "\t";
                if (string == "GET" || string == "POST")
                {
                    if (string == "GET")
                    {
                        Token token(METHOD, "GET");
                        tokens.push_back(token);
                    }
                    else if (string == "POST")
                    {
                        Token token(METHOD, "POST");
                        tokens.push_back(token);
                    }
                } else
                {
                    Token token(KEY, string);
                    tokens.push_back(token);
                }
            }
            else
            {
                std::string Method = sContent[i][0];
                if (Method == "GET" || Method == "POST") // Request Line only
                {
                    if (j == 1)
                    {
                        Token token(PATH, string);
                        tokens.push_back(token);
                        std::cout << token.second << "\t";
                    }
                    else if (j == 2)
                    {
                        Token token(HTTP_VERSION, string);
                        tokens.push_back(token);
                        std::cout << token.second << "\t";
                    }
                } else
                {
                    Token token(VALUE, string);
                    tokens.push_back(token);
                }
            }
        }
        std::cout << std::endl;
    }
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
