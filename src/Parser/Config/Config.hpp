# ifndef CONFIG_HPP
#define CONFIG_HPP

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <cctype>
#include <cstdlib> // for atoi (I will replace it later)

enum TokenType {
    TOKEN_TYPE_WORD,
    TOKEN_TYPE_LBRACE,
    TOKEN_TYPE_RBRACE,
    TOKEN_TYPE_SEMICOLON
};

struct Token {
    std::string value;
    TokenType type;
    Token(std::string v, TokenType t);
};

enum ParserState {
    STATE_GLOBAL,
    STATE_IN_SERVER,
    STATE_IN_LOCATION
};

struct LocationConfig {
    std::string                         uri;
    std::string                         root;
    std::string                         index;
    bool                                autoindex;
    std::vector<std::string>            methods;
    std::pair<int, std::string>         return_loc;
    std::map<std::string, std::string>  cgi_path;

    LocationConfig();
    LocationConfig& operator=(const LocationConfig& other);
};

struct ServerConfig {
    int                             port;
    std::string                     host;
    std::string                     server_name;
    std::string                     client_max_body_size;
    std::string                     root;
    std::string                     index;
    std::map<int, std::string>      error_pages;
    std::vector<LocationConfig>     locations;

    ServerConfig();
    ServerConfig& operator=(const ServerConfig& other);
};

class Config {
    private:
        std::vector<ServerConfig> _servers;

    public:
        void addServer(const ServerConfig& server);
        Config& operator=(const Config& other);
        void debug() const;
};


class Parser {
    private:
        std::vector<Token>  _tokens;
        size_t              _pos;
        ParserState         _state;
        
        Config              _mainConfig;
        ServerConfig        _currentServer;
        LocationConfig      _currentLocation;

        Token consume(TokenType expected);
        Token peek();
        void handleListen();
        void handleServerName();
        void handleErrorPage();
        void handleClientMaxBodySize();
        void handleRoot(bool inLocation);
        void handleIndex(bool inLocation);
        void handleAutoIndex();
        void handleAllowMethods();
        void handleReturn();
        void handleCgiPass();

    public:
        Parser(std::vector<Token> tokens);
        Config parse();
};

std::string readfile(std::string fileName);
std::vector<Token> lexer(std::string fileName);

#endif
