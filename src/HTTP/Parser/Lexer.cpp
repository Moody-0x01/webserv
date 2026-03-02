#include <Server.hpp>

Lexer::Lexer() : content(""), pos(0), lpos(0) {}

void Lexer::tokenize(std::string &content)
{
    this->setContent(content);

    if (content.empty())
        return;

    std::cout << "tokenize:\n" << content << std::endl;
}

void Lexer::setContent(std::string &content)
{
    this->content = content;
}
