#include <Server.hpp>

HttpParser::HttpParser() : currentState(IDLE), lexerInstence(), parent(NULL), targetBodySize(0)
{
}

void HttpParser::handle()
{
    if (!this->validated())
        return;

    if (state() == IDLE)
    {
        size_t endOfHeaders = parent->request_buffer.find("\r\n\r\n");
        if (endOfHeaders != std::string::npos)
        {
            endOfHeaders += 4;
            std::string headersOnly = parent->request_buffer.substr(0, endOfHeaders);
            parent->request_buffer.erase(0, endOfHeaders);
            this->lexerInstence.tokenize(headersOnly);
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
                this->request.setURI(val);
            else if (key == VERSION)
                this->request.setHttpVersion(val);
            else if (key == HEADER_NAME)
            {
                std::string headerkey = val;
                if (i + 1 < tokens.size() && tokens[i + 1].first == HEADER_VALUE)
                {
                    std::string headervalue = tokens[i + 1].second;
                    std::transform(headerkey.begin(), headerkey.end(), headerkey.begin(), ::tolower);
                    request.addHeader(headerkey, headervalue);
                    i++;
                }
            }
        }
		if (isHeaderValueExist("content-length"))
		{
			const std::map<std::string, std::string>& headers = this->request.getHeaders();
			std::map<std::string, std::string>::const_iterator it = headers.find("content-length");
			unsigned int contentLenght = std::atoi(it->second.c_str());
			this->targetBodySize = contentLenght;
			if (this->targetBodySize > 0)
				this->currentState = BODY;
			else
				this->currentState = READY;
		}
		else // Normal GET Request
			this->currentState = READY;
    }

    if (state() == BODY)
    {
        if (this->getRequestBuffer().size() >= this->targetBodySize)
        {
            std::string safe_buffer = this->getRequestBuffer().substr(0, this->targetBodySize);
            this->request.setBody(safe_buffer);
            this->currentState = READY;
        }
    }
}

std::string &HttpParser::getRequestBuffer()
{
    return parent->request_buffer;
}

std::string &HttpParser::getResponseBuffer()
{
    return parent->response_buffer;
}

ParserState HttpParser::state() const
{
    return currentState;
}

void HttpParser::setParent(SocketContext *client)
{
    this->parent = client;
}

SocketContext *HttpParser::getParent() const
{
    return this->parent;
}

Request &HttpParser::getRequestObject()
{
    return this->request;
}

bool HttpParser::validated()
{
    bool validated = false;

    validated = (parent != NULL);
    // more validation shit here idk i may need it :) ...

    return validated;
}

bool HttpParser::isHeaderValueExist(const std::string &key)
{
    return this->request.getHeaders().count(key);
}
