#include "Parser/HTTP/HttpParser.hpp"
#include "HTTP/Response.hpp"
#include "Parser/HTTP/Lexer.hpp"
#include <Server.hpp>
#include <cstddef>
#include <iostream>
#include <string>
#include <utility>

HttpParser::HttpParser() : currentState(IDLE), lexerInstence(), parent(NULL), targetBodySize(-1)
{
}

void HttpParser::handle()
{
    if (this->parent == NULL)
	{
		// Hello?
		std::cout << "Hi\n";
        return;
	}

    if (state() == IDLE)
    {
        size_t endOfHeaders = parent->request_buffer.find("\r\n\r\n");
        if (endOfHeaders != std::string::npos)
        {
            endOfHeaders += 4;
            std::string headersOnly = parent->request_buffer.substr(0, endOfHeaders);
            parent->request_buffer.erase(0, endOfHeaders);
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
                    std::transform(headerkey.begin(), headerkey.end(), headerkey.begin(), ::tolower);
                    request.addHeader(headerkey, headervalue);
                    i++;
                }
            }
        }
		if (isHeaderValueExist("Content-Length"))
		{
			const std::map<std::string, std::string>& headers = this->request.getHeaders();
			std::map<std::string, std::string>::const_iterator it = headers.find("content-length");
			if (it != headers.end())
			{
				this->targetBodySize = std::atoi(it->second.c_str());
				this->request.getHttpRequest().content_length = this->targetBodySize;
			}
			else if (this->request.getMethod() == "POST")
				this->request.setcode(ContentLengthRequired);
		}
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
