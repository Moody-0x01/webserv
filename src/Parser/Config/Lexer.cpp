#include "Config.hpp"

Token::Token(std::string v, TokenType t) : value(v), type(t) {}

std::string readfile(std::string fileName) {
    std::ifstream inputFile(fileName.c_str());
    if (!inputFile.is_open())
        throw std::runtime_error("Error: could not open file");
    
    std::string line, result;
    while (getline(inputFile, line)) {
        result += line + '\n';
    }
    return result;
}

std::vector<Token> lexer(std::string fileName) {
    std::string content = readfile(fileName);
    std::vector<Token> tokens;
    std::string currentToken;

    for (size_t i = 0; i < content.length(); ++i) {
        char c = content[i];

        if (c == '#') { // Comment handling
             if (!currentToken.empty()) {
                tokens.push_back(Token(currentToken, TOKEN_TYPE_WORD));
                currentToken.clear();
            }
            while (i < content.length() && content[i] != '\n') i++;
            continue;
        }

        if (std::isspace(c)) {
            if (!currentToken.empty()) {
                tokens.push_back(Token(currentToken, TOKEN_TYPE_WORD));
                currentToken.clear();
            }
            continue;
        }

        if (c == '{' || c == '}' || c == ';') {
            if (!currentToken.empty()) {
                tokens.push_back(Token(currentToken, TOKEN_TYPE_WORD));
                currentToken.clear();
            }
            TokenType type = (c == '{') ? TOKEN_TYPE_LBRACE : 
                             (c == '}') ? TOKEN_TYPE_RBRACE : TOKEN_TYPE_SEMICOLON;
            tokens.push_back(Token(std::string(1, c), type));
            continue;
        }

        currentToken += c;
    }
    if (!currentToken.empty())
        tokens.push_back(Token(currentToken, TOKEN_TYPE_WORD));
    
    return tokens;
}
