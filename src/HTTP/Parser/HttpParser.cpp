#include <Server.hpp>
#include <string>
#include <unistd.h>
#include <utility>

HttpParser::HttpParser() : currentState(IDLE), lexerInstence(), parent(NULL), targetBodySize(-1)
{
}

ChunkContext::ChunkContext() : hex(""), status(CHUNK_START), remaining(0), prev(0)  {}

std::pair<bool, std::string> HttpParser::getHeaderValue(std::string header_key)
{
	return get_value(this->request.getHeaders(), header_key);
}

void ChunkContext::strip_delimeter(std::vector<char> &data, ChunkStatus new_state, size_t index)
{
	if (index < data.size())
	{
		if (prev == '\r')
		{
			if (data[index] == '\n') {
				this->status = new_state;
				index++;
				prev = 0; // Stripped
			} else
				this->status = CHUNK_ERROR; // Malformed chunk, expected \n after \r
		}
		if (data[index] == '\r') index++;
		if (index >= data.size() || (data[index] != '\n'))
			prev = '\r'; // Not yet found \n
		else
			index++; // Stripped
	}
	if (index <= data.size())
		data.erase(data.begin(), data.begin() + index);
}

void ChunkContext::convert_remaining_into_hex()
{
	std::stringstream ss;
	unsigned long size;

	ss << std::hex << this->hex;
	if (!(ss >> size))
		this->status = CHUNK_ERROR;
}

bool HttpParser::setChunkedEncoding(void) 
{
	std::pair<bool, std::string> pair = this->getHeaderValue("content-encoding");
	if (pair.first)
	{
		this->request.getHttpRequest().ischunked = (pair.second == "chunked");
		return (true);
	}
	return (false);
}

void HttpParser::parseContentLength(void)
{

	std::pair<bool, std::string> pair = this->getHeaderValue("content-length");

	if (pair.first)
	{
		this->targetBodySize = std::atoi(pair.second.c_str());
		this->request.getHttpRequest().content_length = this->targetBodySize;
	}
	this->request.setcode(ContentLengthRequired);
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
		if (this->request.getMethod() == "POST") {
			if (!this->setChunkedEncoding())
				this->parseContentLength();
				
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
