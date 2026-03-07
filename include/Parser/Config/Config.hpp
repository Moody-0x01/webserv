#pragma once
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <cctype>
#include <algorithm>

enum ConfigTokenType {
    CONFIG_TOKEN_TYPE_WORD,
    CONFIG_TOKEN_TYPE_LBRACE,
    CONFIG_TOKEN_TYPE_RBRACE,
    CONFIG_TOKEN_TYPE_SEMICOLON
};

struct ConfigToken {
    std::string value;
    ConfigTokenType type;
    ConfigToken(std::string v, ConfigTokenType t);
};

enum ConfigParserState {
    CONFIG_STATE_GLOBAL,
    CONFIG_STATE_IN_SERVER,
    CONFIG_STATE_IN_LOCATION
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


class ConfigParser {
    private:
        std::vector<ConfigToken>  _tokens;
        size_t                    _pos;
        ConfigParserState         _state;
        
        Config              _mainConfig;
        ServerConfig        _currentServer;
        LocationConfig      _currentLocation;

        ConfigToken consume(ConfigTokenType expected);
        ConfigToken peek();
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
        ConfigParser(std::vector<ConfigToken> tokens);
        Config parse();
};

std::string configReadFile(std::string fileName);
std::vector<ConfigToken> configLexer(std::string fileName);
Config parse_config_file(const std::string& fileName);
