#include <Parser/Config/Config.hpp>

LocationConfig::LocationConfig() : uri(""), root(""), index(""), upload_path(""), autoindex(true), upload_enabled(true) {}

LocationConfig& LocationConfig::operator=(const LocationConfig& other) {
    if (this != &other) {
        uri             = other.uri;
        root            = other.root;
        index           = other.index;
        autoindex       = other.autoindex;
        upload_enabled  = other.upload_enabled;
        upload_path     = other.upload_path;
        methods         = other.methods;
        cgi_path        = other.cgi_path;
        return_loc      = other.return_loc;
    }
    return *this;
}

ServerConfig::ServerConfig() : port(""), host(""), server_name(""), client_max_body_size(1024UL * 1024UL), root(""), index(""), autoindex(true) {}

ServerConfig& ServerConfig::operator=(const ServerConfig& other) {
    if (this != &other) {
        port                  = other.port;
        host                  = other.host;
        server_name           = other.server_name;
        client_max_body_size  = other.client_max_body_size;
        root                  = other.root;
        index                 = other.index;
        autoindex             = other.autoindex;
        mime_types            = other.mime_types;
        error_pages           = other.error_pages;
        locations             = other.locations;
    }
    return *this;
}

ConfigParser::ConfigParser(std::vector<ConfigToken> tokens, std::string &fn) : _tokens(tokens), _pos(0), _state(CONFIG_STATE_GLOBAL), _fn(fn) {}

void Config::addServer(const ServerConfig& server) {
    _servers.push_back(server);
}

Config& Config::operator=(const Config& other) {
    if (this != &other) {
        _servers = other._servers;
    }
    return *this;
}

const std::vector<ServerConfig>& Config::getservers() const {
    return _servers;
}

void Config::debug() const {
    std::cout << "=== CONFIG DUMP ===\n";
    for (size_t i = 0; i < _servers.size(); ++i) {
        const ServerConfig& srv = _servers[i];
        std::cout << "SERVER [" << i << "]\n";
        std::cout << "  Host:                " << srv.host << "\n";
        std::cout << "  Port:                " << srv.port << "\n";
        std::cout << "  Server Name:         " << (srv.server_name.empty() ? "(none)" : srv.server_name) << "\n";
        std::cout << "  Root:                " << (srv.root.empty() ? "(none)" : srv.root) << "\n";
        std::cout << "  Index:               " << (srv.index.empty() ? "(none)" : srv.index) << "\n";
        std::cout << "  Client Max Body Size: " << srv.client_max_body_size << " bytes\n";

        if (!srv.mime_types.empty()) {
            std::cout << "  Mime Types:\n";
            for (std::map<std::string, std::string>::const_iterator it = srv.mime_types.begin(); it != srv.mime_types.end(); ++it)
                std::cout << "    ." << it->first << " -> " << it->second << "\n";
        }

        if (!srv.error_pages.empty()) {
            std::cout << "  Error Pages:\n";
            for (std::map<size_t, std::string>::const_iterator it = srv.error_pages.begin(); it != srv.error_pages.end(); ++it)
                std::cout << "    " << it->first << " -> " << it->second << "\n";
        }

        for (size_t j = 0; j < srv.locations.size(); ++j) {
            const LocationConfig& loc = srv.locations[j];
            std::cout << "  LOCATION [" << loc.uri << "]\n";
            std::cout << "    Root:           " << (loc.root.empty() ? "(none)" : loc.root) << "\n";
            std::cout << "    Index:          " << (loc.index.empty() ? "(none)" : loc.index) << "\n";
            std::cout << "    Auto Index:     " << (loc.autoindex ? "on" : "off") << "\n";
            std::cout << "    Upload Enabled: " << (loc.upload_enabled ? "on" : "off") << "\n";
            std::cout << "    Upload Path:    " << (loc.upload_path.empty() ? "(none)" : loc.upload_path) << "\n";

            if (!loc.methods.empty()) {
                std::cout << "    Allow Methods:  ";
                for (size_t m = 0; m < loc.methods.size(); ++m)
                    std::cout << loc.methods[m] << (m + 1 < loc.methods.size() ? ", " : "\n");
            }

            if (!loc.return_loc.second.empty())
                std::cout << "    Return:         " << loc.return_loc.first << " " << loc.return_loc.second << "\n";

            if (!loc.cgi_path.empty()) {
                std::cout << "    CGI Pass:\n";
                for (std::map<std::string, std::string>::const_iterator it = loc.cgi_path.begin(); it != loc.cgi_path.end(); ++it)
                    std::cout << "      " << it->first << " -> " << it->second << "\n";
            }
        }
        std::cout << "-------------------\n";
    }
}

size_t parseNumber(const std::string &value) {
    std::istringstream iss(value);
    size_t parsedNumber;
    iss >> parsedNumber;

    if (iss.fail() || !iss.eof())
        return std::string::npos;
    return parsedNumber;
}

bool isdigits(const std::string &str) {
    if (str.size() == 0) return false;
    for (size_t i = 0; i < str.size(); i++) {
        if (!std::isdigit(str[i]))
            return false;
    }
    return true;
}

ConfigToken ConfigParser::consume(ConfigTokenType expected) {
    if (_pos >= _tokens.size()) errorLogger("Unexpected End of Stream", _tokens[_tokens.size() - 1].line);
    if (_tokens[_pos].type != expected) errorLogger("Unexpected token \"" + _tokens[_pos].value + "\"", _tokens[_pos].line);
    return _tokens[_pos++];
}

ConfigToken ConfigParser::peek() {
     if (_pos >= _tokens.size()) errorLogger("Unexpected End of Stream", _tokens[_tokens.size() - 1].line);
     return _tokens[_pos];
}

void ConfigParser::handleListen() {
    consume(CONFIG_TOKEN_TYPE_WORD);
    ConfigToken t = consume(CONFIG_TOKEN_TYPE_WORD);
    size_t colInd = t.value.find(':');
    if (colInd == std::string::npos)
        errorLogger("Unexpected Parameter in listen \"" + t.value + "\"", t.line);
    std::string host = t.value.substr(0, colInd);
    std::string port = t.value.substr(colInd + 1, t.value.size());
    if (!isdigits(port))
        errorLogger("Unexpected Parameter in listen \"" + t.value + "\"", t.line);
    _currentServer.host = host;
    _currentServer.port = port;
    consume(CONFIG_TOKEN_TYPE_SEMICOLON);
}

void ConfigParser::handleServerName() {
    consume(CONFIG_TOKEN_TYPE_WORD);
    _currentServer.server_name = consume(CONFIG_TOKEN_TYPE_WORD).value;
    consume(CONFIG_TOKEN_TYPE_SEMICOLON);
}

void ConfigParser::handleErrorPage() {
    consume(CONFIG_TOKEN_TYPE_WORD);
    std::vector<size_t> codes;
    while (_pos < _tokens.size() && _tokens[_pos].type == CONFIG_TOKEN_TYPE_WORD) {
        ConfigToken t = peek();
        if (isdigits(t.value)) {
            consume(CONFIG_TOKEN_TYPE_WORD);
            size_t tmp = parseNumber(t.value);
            if (tmp < 400 || tmp > 599)
                errorLogger("Invalid HTML Error Code.", t.line);
            codes.push_back(tmp);
        }
        else
            break;
    }
    if (codes.empty())
        errorLogger("error_page Missing Codes.", _tokens[_pos].line);
    ConfigToken pathToken = consume(CONFIG_TOKEN_TYPE_WORD);
    for (size_t i = 0; i < codes.size(); ++i) {
        _currentServer.error_pages[codes[i]] = pathToken.value;
    }
    consume(CONFIG_TOKEN_TYPE_SEMICOLON);
}

size_t ConfigParser::parseMaxBodySize(const std::string &value, size_t line) {
    if (value.empty())
        errorLogger("Invalid Token in \"client_max_body_size\"", line);
    std::string numberPart = value;
    size_t multiplier = 1;
    char lastChar = value[value.size() - 1];

    if (!std::isdigit(lastChar)) {
        switch (lastChar) {
            case 'K': case 'k':
                multiplier = 1024UL;
                break;
            case 'M': case 'm':
                multiplier = 1024UL * 1024UL;
                break;
            case 'G': case 'g':
                multiplier = 1024UL * 1024UL * 1024UL;
                break;
            default:
                errorLogger(std::string("client_max_body_size: Invalid Suffix '") + lastChar + "'", line);
        }
        numberPart = value.substr(0, value.size() - 1);
    }
    if (numberPart.empty())
        errorLogger("Invalid Token in \"client_max_body_size\"", line);
    if (!isdigits(numberPart))
        errorLogger("Invalid Token in \"client_max_body_size\"", line);

    size_t number = parseNumber(numberPart);
    size_t max_size = -1;
    if (number > max_size / multiplier)
        errorLogger("Invalid Token in \"client_max_body_size\"", line);
    return number * multiplier;
}

void ConfigParser::handleClientMaxBodySize() {
    consume(CONFIG_TOKEN_TYPE_WORD);
    ConfigToken t = consume(CONFIG_TOKEN_TYPE_WORD);
    _currentServer.client_max_body_size = parseMaxBodySize(t.value, t.line);
    consume(CONFIG_TOKEN_TYPE_SEMICOLON);
}

void ConfigParser::handleRoot(bool inLocation) {
    consume(CONFIG_TOKEN_TYPE_WORD);
    ConfigToken t = consume(CONFIG_TOKEN_TYPE_WORD);
    if (inLocation) _currentLocation.root = t.value;
    else _currentServer.root = t.value;
    consume(CONFIG_TOKEN_TYPE_SEMICOLON);
}

void ConfigParser::handleIndex(bool inLocation) {
    consume(CONFIG_TOKEN_TYPE_WORD);
    ConfigToken t = consume(CONFIG_TOKEN_TYPE_WORD);
    if (inLocation) _currentLocation.index = t.value;
    else _currentServer.index = t.value;
    consume(CONFIG_TOKEN_TYPE_SEMICOLON);
}

void ConfigParser::handleAutoIndex(bool inLocation) {
    consume(CONFIG_TOKEN_TYPE_WORD);
    ConfigToken t = consume(CONFIG_TOKEN_TYPE_WORD);
    if (t.value == "on")
    {
        if (inLocation) _currentLocation.autoindex = true;
        else _currentServer.autoindex = true;
    }
    else if (t.value == "off")
    {
        if (inLocation) _currentLocation.autoindex = false;
        else _currentServer.autoindex = false;
    }
    else
        errorLogger("Invalid Auto Index value \"" + t.value + "\"", t.line);
    consume(CONFIG_TOKEN_TYPE_SEMICOLON);
}

void ConfigParser::handleUploadEnabled() {
    consume(CONFIG_TOKEN_TYPE_WORD);
    ConfigToken t = consume(CONFIG_TOKEN_TYPE_WORD);
    if (t.value == "on")
        _currentLocation.upload_enabled = true;
    else if (t.value == "off")
        _currentLocation.upload_enabled = false;
    else
        errorLogger("Invalid Upload_Enabled value \"" + t.value + "\"", t.line);
    consume(CONFIG_TOKEN_TYPE_SEMICOLON);
}

void ConfigParser::handleUploadPath() {
    consume(CONFIG_TOKEN_TYPE_WORD);
    _currentLocation.upload_path = consume(CONFIG_TOKEN_TYPE_WORD).value;
    consume(CONFIG_TOKEN_TYPE_SEMICOLON);
}

void ConfigParser::handleAllowMethods() {
    consume(CONFIG_TOKEN_TYPE_WORD);
    ConfigToken t = peek();
    while (_pos < _tokens.size() && _tokens[_pos].type == CONFIG_TOKEN_TYPE_WORD) {
        t = consume(CONFIG_TOKEN_TYPE_WORD);
        if (t.value != "GET" && t.value != "POST" && t.value != "DELETE")
            errorLogger("Invalid method \"" + t.value + "\"", t.line);
        if (std::find(_currentLocation.methods.begin(), _currentLocation.methods.end(), t.value) == _currentLocation.methods.end())
            _currentLocation.methods.push_back(t.value);
    }
    if (_currentLocation.methods.empty())
        errorLogger("allow_methods can't be empty.", t.line);
    consume(CONFIG_TOKEN_TYPE_SEMICOLON);
}

void ConfigParser::handleMimeTypes() {
    consume(CONFIG_TOKEN_TYPE_WORD);

    bool has_any = false;
    while (_pos < _tokens.size() && _tokens[_pos].type != CONFIG_TOKEN_TYPE_SEMICOLON) {
        ConfigToken mime = consume(CONFIG_TOKEN_TYPE_WORD);
        if (_pos >= _tokens.size() || _tokens[_pos].type == CONFIG_TOKEN_TYPE_SEMICOLON)
            errorLogger("mime_types expects pairs: <mime> <ext>", mime.line);
        ConfigToken ext = consume(CONFIG_TOKEN_TYPE_WORD);
        _currentServer.mime_types[ext.value] = mime.value;
        has_any = true;
    }

    if (!has_any)
        errorLogger("mime_types can't be empty", peek().line);
    consume(CONFIG_TOKEN_TYPE_SEMICOLON);
}

void ConfigParser::handleReturn() {
    consume(CONFIG_TOKEN_TYPE_WORD);
    size_t code = 302;
    std::string url = "";
    ConfigToken t = consume(CONFIG_TOKEN_TYPE_WORD);
    
    if (isdigits(t.value)) {
        code = parseNumber(t.value);
        if (code > 399 || code < 300)
            errorLogger("Invalid Redirect Codes \"" + t.value + "\"", t.line);
        url = consume(CONFIG_TOKEN_TYPE_WORD).value;
    }
    else
        url = t.value;
    _currentLocation.return_loc = std::make_pair(code, url);
    consume(CONFIG_TOKEN_TYPE_SEMICOLON);
}

void ConfigParser::verifyExt(ConfigToken &t) {
    std::string ext = t.value;
    // if (ext != ".py")
    //     errorLogger("Unsupported CGI extension \"" + ext + "\"", t.line);
}

void ConfigParser::handleCgiPass() {
    consume(CONFIG_TOKEN_TYPE_WORD);
    ConfigToken ext = consume(CONFIG_TOKEN_TYPE_WORD);
    ConfigToken bin = consume(CONFIG_TOKEN_TYPE_WORD);
    verifyExt(ext);
    _currentLocation.cgi_path[ext.value] = bin.value;
    consume(CONFIG_TOKEN_TYPE_SEMICOLON);
}

void ConfigParser::errorLogger(std::string specs, size_t line) {
    std::stringstream ss;
    ss << line;

    throw std::runtime_error("[Config-Error] " + specs + " in " + _fn + ":" + ss.str());
}

void ConfigParser::validateServer(const ServerConfig &server) {
    if (server.host.empty() || server.port.empty())
        errorLogger("Invalid or not existed listen rule.", 0);
}

Config ConfigParser::parse() {
    while (_pos < _tokens.size()) {
        ConfigToken t = peek();

        switch (_state) {
            case CONFIG_STATE_GLOBAL:
                if (t.value == "server") {
                    consume(CONFIG_TOKEN_TYPE_WORD);
                    consume(CONFIG_TOKEN_TYPE_LBRACE);
                    _state = CONFIG_STATE_IN_SERVER;
                    _currentServer = ServerConfig();
                }
                else
                    errorLogger("Unexpected token in Global \"" + t.value + "\"", t.line);
                break;

            case CONFIG_STATE_IN_SERVER:
                if (t.type == CONFIG_TOKEN_TYPE_RBRACE) {
                    consume(CONFIG_TOKEN_TYPE_RBRACE);
                    validateServer(_currentServer);
                    _mainConfig.addServer(_currentServer);
                    _state = CONFIG_STATE_GLOBAL;
                }
                else if (t.value == "listen")               handleListen();
                else if (t.value == "server_name")          handleServerName();
                else if (t.value == "client_max_body_size") handleClientMaxBodySize();
                else if (t.value == "error_page")           handleErrorPage();
                else if (t.value == "root")                 handleRoot(false);
                else if (t.value == "index")                handleIndex(false);
                else if (t.value == "auto_index")           handleAutoIndex(false);
                else if (t.value == "mime_types")           handleMimeTypes();
                else if (t.value == "location") {
                    consume(CONFIG_TOKEN_TYPE_WORD);
                    _currentLocation = LocationConfig();
                    _currentLocation.uri = consume(CONFIG_TOKEN_TYPE_WORD).value;
                    consume(CONFIG_TOKEN_TYPE_LBRACE);
                    _state = CONFIG_STATE_IN_LOCATION;
                }
                else errorLogger("Unexpected token in Server \"" + t.value + "\"", t.line);
                break;

            case CONFIG_STATE_IN_LOCATION:
                if (t.type == CONFIG_TOKEN_TYPE_RBRACE) {
                    consume(CONFIG_TOKEN_TYPE_RBRACE);
                    _currentServer.locations.push_back(_currentLocation);
                    _state = CONFIG_STATE_IN_SERVER;
                }
                else if (t.value == "root")            handleRoot(true);
                else if (t.value == "index")           handleIndex(true);
                else if (t.value == "auto_index")      handleAutoIndex(true);
                else if (t.value == "upload_enabled")  handleUploadEnabled();
                else if (t.value == "upload_path")     handleUploadPath();
                else if (t.value == "allow_methods")   handleAllowMethods();
                else if (t.value == "return")          handleReturn();
                else if (t.value == "cgi_pass")        handleCgiPass();
                else errorLogger("Unexpected token in Location \"" + t.value + "\"", t.line);
                break;
        }
    }
    if (_state != CONFIG_STATE_GLOBAL)
        errorLogger("Unexpected end of stream", _tokens[_tokens.size() - 1].line);
    return _mainConfig;
}

std::string ServerConfig::label() const {
	return (this->server_name + ":" + this->port);
}

Config parse_config_file(std::string fileName) {
    std::vector<ConfigToken> tokens = configLexer(fileName);
    ConfigParser parser(tokens, fileName);
    return parser.parse();
}
