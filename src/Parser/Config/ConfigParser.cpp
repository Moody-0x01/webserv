#include "Config.hpp"

LocationConfig::LocationConfig() : uri(""), root(""), index(""), autoindex(false) {}


LocationConfig& LocationConfig::operator=(const LocationConfig& other) {
    if (this != &other) {
        uri        = other.uri;
        root       = other.root;
        index      = other.index;
        autoindex  = other.autoindex;
        methods    = other.methods;
    }
    return *this;
}

ServerConfig::ServerConfig() : port(80), host("127.0.0.1"), root(""), index("") {}

ServerConfig& ServerConfig::operator=(const ServerConfig& other) {
    if (this != &other) {
        port                  = other.port;
        host                  = other.host;
        server_name           = other.server_name;
        client_max_body_size  = other.client_max_body_size;
        root                  = other.root;
        index                 = other.index;
        error_pages           = other.error_pages;
        locations             = other.locations;
    }
    return *this;
}

Parser::Parser(std::vector<Token> tokens) : _tokens(tokens), _pos(0), _state(STATE_GLOBAL) {}

void Config::addServer(const ServerConfig& server) {
    _servers.push_back(server);
}

Config& Config::operator=(const Config& other) {
    if (this != &other) {
        _servers = other._servers;
    }
    return *this;
}

void Config::debug() const {
    std::cout << "=== CONFIG DUMP ===\n";
    for (size_t i = 0; i < _servers.size(); ++i) {
        std::cout << "SERVER [" << i << "]\n";
        std::cout << "  Port: " << _servers[i].port << "\n";
        std::cout << "  Root: " << _servers[i].root << "\n";
        std::cout << "  Index: " << _servers[i].index << "\n";
        std::cout << "  Server Name: " << _servers[i].server_name << "\n";
        std::cout << "  client max body size: " << _servers[i].client_max_body_size << "\n";
        for (size_t j = 0; j < _servers[i].locations.size(); ++j) {
            std::cout << "  LOCATION [" << _servers[i].locations[j].uri << "]\n";
            std::cout << "    Root: " << _servers[i].locations[j].root << "\n";
            std::cout << "    Index: " << _servers[i].locations[j].index << "\n";
        }
        std::cout << "-------------------\n";
    }
}

Token Parser::consume(TokenType expected) {
    if (_pos >= _tokens.size()) throw std::runtime_error("Unexpected End of Stream");
    if (_tokens[_pos].type != expected) throw std::runtime_error("Syntax Error: Unexpected token " + _tokens[_pos].value);
    return _tokens[_pos++];
}

Token Parser::peek() {
     if (_pos >= _tokens.size()) throw std::runtime_error("Unexpected End of Stream");
     return _tokens[_pos];
}

void Parser::handleListen() {
    consume(TOKEN_TYPE_WORD);
    Token t = consume(TOKEN_TYPE_WORD);
    size_t colInd = t.value.find(':');
    std::string host = t.value.substr(0, colInd);
    std::string port = t.value.substr(colInd + 1, t.value.size());
    _currentServer.host = host;
    _currentServer.port = std::atoi(port.c_str());
    consume(TOKEN_TYPE_SEMICOLON);
}

void Parser::handleServerName() {
    consume(TOKEN_TYPE_WORD);
    _currentServer.server_name = consume(TOKEN_TYPE_WORD).value;
    consume(TOKEN_TYPE_SEMICOLON);
}

void Parser::handleClientMaxBodySize() {
    consume(TOKEN_TYPE_WORD);
    _currentServer.client_max_body_size = consume(TOKEN_TYPE_WORD).value;
    consume(TOKEN_TYPE_SEMICOLON);
}

void Parser::handleRoot(bool inLocation) {
    consume(TOKEN_TYPE_WORD);
    Token t = consume(TOKEN_TYPE_WORD);
    if (inLocation) _currentLocation.root = t.value;
    else _currentServer.root = t.value;
    consume(TOKEN_TYPE_SEMICOLON);
}

void Parser::handleIndex(bool inLocation) {
    consume(TOKEN_TYPE_WORD);
    Token t = consume(TOKEN_TYPE_WORD);
    if (inLocation) _currentLocation.index = t.value;
    else _currentServer.index = t.value;
    consume(TOKEN_TYPE_SEMICOLON);
}

void Parser::handleAutoIndex() {
    consume(TOKEN_TYPE_WORD);
    if (consume(TOKEN_TYPE_WORD).value == "on")
        _currentLocation.autoindex = true;
    else
        _currentLocation.autoindex = false;
    consume(TOKEN_TYPE_SEMICOLON);
}

Config Parser::parse() {
    while (_pos < _tokens.size()) {
        Token t = peek();

        switch (_state) {
            case STATE_GLOBAL:
                if (t.value == "server") {
                    consume(TOKEN_TYPE_WORD);
                    consume(TOKEN_TYPE_LBRACE);
                    _state = STATE_IN_SERVER;
                    _currentServer = ServerConfig();
                } else {
                    throw std::runtime_error("Unexpected token in Global: " + t.value);
                }
                break;

            case STATE_IN_SERVER:
                if (t.type == TOKEN_TYPE_RBRACE) {
                    consume(TOKEN_TYPE_RBRACE);
                    _mainConfig.addServer(_currentServer);
                    _state = STATE_GLOBAL;
                }
                else if (t.value == "listen") handleListen();
                else if (t.value == "server_name") handleServerName();
                else if (t.value == "client_max_body_size") handleClientMaxBodySize();
                else if (t.value == "root") handleRoot(false);
                else if (t.value == "index") handleIndex(false);
                else if (t.value == "location") {
                    consume(TOKEN_TYPE_WORD);
                    _currentLocation = LocationConfig();
                    _currentLocation.uri = consume(TOKEN_TYPE_WORD).value;
                    consume(TOKEN_TYPE_LBRACE);
                    _state = STATE_IN_LOCATION;
                }
                else throw std::runtime_error("Unknown directive in Server: " + t.value);
                break;

            case STATE_IN_LOCATION:
                if (t.type == TOKEN_TYPE_RBRACE) {
                    consume(TOKEN_TYPE_RBRACE);
                    _currentServer.locations.push_back(_currentLocation);
                    _state = STATE_IN_SERVER;
                }
                else if (t.value == "root") handleRoot(true);
                else if (t.value == "index") handleIndex(true);
                else if (t.value == "auto_index") handleAutoIndex();
                else throw std::runtime_error("Unknown directive in Location: " + t.value);
                break;
        }
    }
    return _mainConfig;
}
