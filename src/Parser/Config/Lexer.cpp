#include "Config.hpp"

ConfigToken::ConfigToken(std::string v, ConfigTokenType t) : value(v), type(t) {}

std::string configReadFile(std::string fileName) {
    std::ifstream inputFile(fileName.c_str());
    if (!inputFile.is_open())
        throw std::runtime_error("Error: could not open file");
    
    std::string line, result;
    while (getline(inputFile, line)) {
        result += line + '\n';
    }
    return result;
}

std::vector<ConfigToken> configLexer(std::string fileName) {
    std::string content = configReadFile(fileName);
    std::vector<ConfigToken> tokens;
    std::string currentToken;

    for (size_t i = 0; i < content.length(); ++i) {
        char c = content[i];

        if (c == '#') { // Comment handling
             if (!currentToken.empty()) {
                tokens.push_back(ConfigToken(currentToken, CONFIG_TOKEN_TYPE_WORD));
                currentToken.clear();
            }
            while (i < content.length() && content[i] != '\n') i++;
            continue;
        }

        if (std::isspace(c)) {
            if (!currentToken.empty()) {
                tokens.push_back(ConfigToken(currentToken, CONFIG_TOKEN_TYPE_WORD));
                currentToken.clear();
            }
            continue;
        }

        if (c == '{' || c == '}' || c == ';') {
            if (!currentToken.empty()) {
                tokens.push_back(ConfigToken(currentToken, CONFIG_TOKEN_TYPE_WORD));
                currentToken.clear();
            }
            ConfigTokenType type = (c == '{') ? CONFIG_TOKEN_TYPE_LBRACE : 
                             (c == '}') ? CONFIG_TOKEN_TYPE_RBRACE : CONFIG_TOKEN_TYPE_SEMICOLON;
            tokens.push_back(ConfigToken(std::string(1, c), type));
            continue;
        }

        currentToken += c;
    }
    if (!currentToken.empty())
        tokens.push_back(ConfigToken(currentToken, CONFIG_TOKEN_TYPE_WORD));
    
    return tokens;
}
