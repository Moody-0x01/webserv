#include <Server.hpp>

HttpParser::HttpParser() : currentState(IDLE), lexerInstence(), parent(NULL), targetBodySize(-1)
{
}

header_iterator HttpParser::getHeaderValue(std::string header_key)
{
	return search(this->request.getHeaders(), header_key);
}

void HttpParser::setChunkedEncoding(void) 
{
	const std::map<std::string, std::string>& headers = this->request.getHeaders();
}

bool HttpParser::parseContentLength(void)
{
	const std::map<std::string, std::string>& headers = this->request.getHeaders();

	std::map<std::string, std::string>::const_iterator it = headers.find("content-length");
	if (it != headers.end())
	{
		this->targetBodySize = std::atoi(it->second.c_str());
		this->request.getHttpRequest().content_length = this->targetBodySize;
		return (true);
	}
	else if (this->request.getMethod() == "POST")
		this->request.setcode(ContentLengthRequired);
	return (false);
}

void HttpParser::handle()
{
    if (this->parent == NULL)
        return;
    if (state() == IDLE)
    {
		std::vector<char>::iterator it = search(parent->_buffer, "\r\n\r\n");
        if (it != parent->_buffer.end())
        {
			size_t endOfHeaders = ((it + 4) - parent->_buffer.begin() - 1);
            std::string headersOnly = collect(parent->_buffer, endOfHeaders);
            parent->_buffer.erase(parent->_buffer.begin(),
					parent->_buffer.begin() + endOfHeaders);
			this->getRequestObject().setBody(&parent->_buffer);
            this->lexerInstence.tokenize(headersOnly);
            if (lexerInstence.isBadRequest()) // for now am doing it from here.
            {
                this->request.setcode(BadRequest);
                this->currentState = READY;
                return;
            }
            else 
                this->currentState = HEADERS_DONE;
        }
    }

    if (state() == HEADERS_DONE)
    {
        std::vector<Token> &tokens = lexerInstence.getTokens();
        for (size_t i = 0; i < tokens.size(); i++)
        {
            HttpTokenType &key = tokens[i].first;
            std::string &val = tokens[i].second;
            if (key == METHOD)
                this->request.setMethod(val);
            else if (key == URI)
            {
                this->parseParams(tokens[i].second);
                this->request.setURI(val); 
            }
            else if (key == VERSION)
                this->request.setHttpVersion(val);
            else if (key == HEADER_NAME)
            {
                std::string headerkey = val;
                if (i + 1 < tokens.size() && tokens[i + 1].first == HEADER_VALUE)
                {
                    std::string headervalue = tokens[i + 1].second;
                    request.addHeader(headerkey, headervalue);
                    i++;
                }
            }
        }
		// TODO: Check for Content-Encoding
		// this->fetch()
		this->parseContentLength();

		this->currentState = READY;
    }
}

void HttpParser::parseParams(std::string &uri)
{
    std::map<std::string, std::string> &params = this->getRequestObject().getHttpRequest().params;
    params.clear();
    size_t pos = uri.find('?');
    if (pos == std::string::npos)
        return;
    std::string query_string = uri.substr(pos + 1);
    this->request.getHttpRequest().query_string = query_string;
    uri = uri.substr(0, pos);
    size_t start = 0;
    while (start < query_string.length()) {
        size_t amp = query_string.find('&', start);
        if (amp == std::string::npos)
            amp = query_string.length();
        std::string pair = query_string.substr(start,  amp - start);
        size_t equalp = pair.find('=');
        if (equalp != std::string::npos)
        {
            std::string key = pair.substr(0, equalp);
            std::string value = pair.substr(equalp + 1);
            params[key] = value;
        }
        start = amp + 1;
    }
}

ParserState HttpParser::state() const
{
    return currentState;
}

void HttpParser::setParent(Client *client)
{
    this->parent = client;
}

Client *HttpParser::getParent() const
{
    return this->parent;
}

Request &HttpParser::getRequestObject()
{
    return this->request;
}


bool HttpParser::isHeaderValueExist(const std::string &key)
{
    return this->request.getHeaders().count(key);
}
