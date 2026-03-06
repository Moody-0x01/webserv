#pragma once
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <cctype>
#include <algorithm>

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
    std::string                         upload_path;
    bool                                autoindex;
    bool                                upload_enabled;
    std::vector<std::string>            methods;
    std::pair<size_t, std::string>      return_loc;
    std::map<std::string, std::string>  cgi_path;

    LocationConfig();
    LocationConfig& operator=(const LocationConfig& other);
};

struct ServerConfig {
    size_t                       port;
    std::string                  host;
    std::string                  server_name;
    size_t                       client_max_body_size;
    std::string                  root;
    std::string                  index;
    std::map<size_t, std::string>   error_pages;
    std::vector<LocationConfig>  locations;

    ServerConfig();
    ServerConfig& operator=(const ServerConfig& other);
};

class Config {
    private:
        std::vector<ServerConfig> _servers;

    public:
        void addServer(const ServerConfig& server);
        Config& operator=(const Config& other);
        const std::vector<ServerConfig>& getservers() const;
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
        void handleUploadEnabled();
        void handleUploadPath();
        void handleAllowMethods();
        void handleReturn();
        void handleCgiPass();

    public:
        Parser(std::vector<Token> tokens);
        Config parse();
};

std::string readfile(std::string fileName);
std::vector<Token> lexer(std::string fileName);
