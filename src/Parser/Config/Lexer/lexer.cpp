#include <string>
#include <fstream>
#include <iostream>
#include <vector>
#include <sstream>

enum TokenType {
    TOKEN_TYPE_WORD,
    TOKEN_TYPE_LBRACE,
    TOKEN_TYPE_RBRACE,
    TOKEN_TYPE_SEMICOLON
};

struct Token {
    std::string value;
    TokenType type;
};

std::string readfile(std::string fileName) {
    std::string line;
    std::string result;
    std::ifstream inputFile;

    inputFile.open(fileName.c_str(), std::ios::in);
    while (getline(inputFile, line)) {
        result += line;
        if (!inputFile.eof())
            result += '\n';
    }

    return result;
}

std::vector<Token> tokenize(const std::string& content) {
    std::vector<Token> tokens;
    std::string currentToken;

    for (size_t i = 0; i < content.length(); ++i) {
        char c = content[i];

        if (std::isspace(c)) {
            if (!currentToken.empty()) {
                tokens.push_back({currentToken, TOKEN_TYPE_WORD});
                currentToken.clear();
            }
            continue;
        }

        if (c == '{' || c == '}' || c == ';') {
            if (!currentToken.empty()) {
                tokens.push_back({currentToken, TOKEN_TYPE_WORD});
                currentToken.clear();
            }

            TokenType type;
            if (c == '{') type = TOKEN_TYPE_LBRACE;
            else if (c == '}') type = TOKEN_TYPE_RBRACE;
            else type = TOKEN_TYPE_SEMICOLON;

            tokens.push_back({std::string(1, c), type});
            continue;
        }
        currentToken += c;
    }

    if (!currentToken.empty()) {
        tokens.push_back({currentToken, TOKEN_TYPE_WORD});
    }

    return tokens;
}

std::vector<Token> lexer(std::string fn) {
    std::string fileContent = readfile(fn);
    std::vector<Token> tokens = tokenize(fileContent);
    return tokens;
}

int main(int ac, char **av)
{
    std::vector<Token> tokens = lexer(std::string(av[1]));
    for (std::vector<Token>::iterator it = tokens.begin(); it != tokens.end(); ++it) {
        std::cout << "Token: " << it->value << "\t type:" << "W{};"[it->type] << "\n";
    }
}
