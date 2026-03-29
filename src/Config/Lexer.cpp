#include <Parser/Config/Config.hpp>

ConfigToken::ConfigToken(std::string v, ConfigTokenType t, size_t l) : value(v), type(t), line(l) {}

std::string configReadFile(const std::string& fileName) {
    std::ifstream inputFile(fileName.c_str());
    if (!inputFile.is_open())
        throw std::runtime_error("[Config-Error] could not open file " + fileName);
    
    std::string line, result;
    while (getline(inputFile, line)) {
        result += line + '\n';
    }
    return result;
}

std::vector<ConfigToken> configLexer(const std::string& fileName) {
    std::string content = configReadFile(fileName);
    std::vector<ConfigToken> tokens;
    std::string currentToken;
    size_t lineNumber = 1;

    for (size_t i = 0; i < content.length(); ++i) {
        char c = content[i];

        if (c == '#') {
             if (!currentToken.empty()) {
                tokens.push_back(ConfigToken(currentToken, CONFIG_TOKEN_TYPE_WORD, lineNumber));
                currentToken.clear();
            }
            while (i < content.length() && content[i] != '\n') i++;
            lineNumber++;
            continue;
        }

        if (std::isspace(c)) {
            if (!currentToken.empty()) {
                tokens.push_back(ConfigToken(currentToken, CONFIG_TOKEN_TYPE_WORD, lineNumber));
                currentToken.clear();
            }
            if (c == '\n') lineNumber++;
            continue;
        }

        if (c == '{' || c == '}' || c == ';') {
            if (!currentToken.empty()) {
                tokens.push_back(ConfigToken(currentToken, CONFIG_TOKEN_TYPE_WORD, lineNumber));
                currentToken.clear();
            }
            ConfigTokenType type = (c == '{') ? CONFIG_TOKEN_TYPE_LBRACE : 
                             (c == '}') ? CONFIG_TOKEN_TYPE_RBRACE : CONFIG_TOKEN_TYPE_SEMICOLON;
            tokens.push_back(ConfigToken(std::string(1, c), type, lineNumber));
            continue;
        }

        currentToken += c;
    }
    if (!currentToken.empty())
        tokens.push_back(ConfigToken(currentToken, CONFIG_TOKEN_TYPE_WORD, lineNumber));

    return tokens;
}
