#pragma once
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <cctype>
#include <algorithm>

# define DEFAULT_CONF "./conf/default.conf"
enum ConfigTokenType {
    CONFIG_TOKEN_TYPE_WORD,
    CONFIG_TOKEN_TYPE_LBRACE,
    CONFIG_TOKEN_TYPE_RBRACE,
    CONFIG_TOKEN_TYPE_SEMICOLON
};

struct ConfigToken {
    std::string         value;
    ConfigTokenType     type;
    size_t              line;
    ConfigToken(std::string v, ConfigTokenType t, size_t l);
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
	std::string                  port;
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

        std::string& _fn;

        ConfigToken consume(ConfigTokenType expected);
        ConfigToken peek();
        void        handleListen();
        void        handleServerName();
        void        handleErrorPage();
        size_t      parseMaxBodySize(const std::string &value, size_t line);
        void        handleClientMaxBodySize();
        void        handleRoot(bool inLocation);
        void        handleIndex(bool inLocation);
        void        handleAutoIndex();
        void        handleUploadEnabled();
        void        handleUploadPath();
        void        handleAllowMethods();
        void        handleReturn();
        void        verifyExt(ConfigToken &t);
        void        handleCgiPass();
        void        errorLogger(std::string specs, size_t line);
        void        validateServer(const ServerConfig &server);

    public:
        ConfigParser(std::vector<ConfigToken> tokens, std::string &fn);
        Config parse();
};

std::string configReadFile(const std::string& fileName);
std::vector<ConfigToken> configLexer(const std::string& fileName);
Config parse_config_file(std::string fileName);
