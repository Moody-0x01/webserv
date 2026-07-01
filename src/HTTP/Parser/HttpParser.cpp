#include <Server.hpp>
#include <cstddef>
#include <iostream>
#include <string>
#include <unistd.h>
#include <utility>
#include <vector>

HttpParser::HttpParser() : currentState(IDLE), lexerInstence(), parent(NULL), targetBodySize(-1)
{
}

ChunkContext::ChunkContext() : hex(""), status_mask(CHUNK_START), remaining(0), cursor(0), prev(0)  {}

std::pair<bool, std::string> HttpParser::getHeaderValue(std::string header_key)
{
	return utility::get_value(this->request.getHeaders(), header_key);
}

bool ChunkContext::is_corrupted(void)
{
	return (this->status_mask & CHUNK_ERROR);
}

bool ChunkContext::is_reading_size(void)
{
	return (this->status_mask & CHUNK_START || this->status_mask & CHUNK_SIZE);
}

bool ChunkContext::just_started(void)
{
	return (this->status_mask & CHUNK_START);
}

bool ChunkContext::is_reading_data(void)
{
	return (this->status_mask & CHUNK_DATA);
}

bool ChunkContext::is_reading_crlf(void)
{
	return (this->status_mask & CHUNK_TRAILER);
}

bool ChunkContext::is_done(void) const
{
	return (this->status_mask & CHUNK_COMPLETE);
}

void ChunkContext::log_buffer(std::vector<char> &buffer)
{
	std::cout << "|";
	for (size_t i = 0; i < buffer.size(); i++)
	{
		if (buffer[i] == '\r')
			std::cout << "\\r";
		else if (buffer[i] == '\n')
			std::cout << "\\n";
		else
			std::cout << buffer[i];
	}
	std::cout << "|\n";
}

void ChunkContext::skip_crlf(std::vector<char> &buffer)
{
	switch (buffer[this->cursor])
	{
		case CR: {
			this->prev = buffer[this->cursor];
		} break ;
		case LF: {
			if (!this->prev)
			{
				this->status_mask = CHUNK_ERROR; // Malformed chunk, expected \r\n after data, 500, BadRequest :(
				return ;
			}
			if (this->status_mask & CHUNK_DATA)
				this->status_mask = CHUNK_SIZE;
			else if (this->status_mask & (CHUNK_SIZE|CHUNK_START))
				this->status_mask = CHUNK_DATA;
			this->prev = 0;
		} break ;
		case ';': {
			if (this->status_mask & (CHUNK_SIZE | CHUNK_START))
				while (this->cursor < buffer.size() && buffer[this->cursor] != CR) this->cursor++;
			return ;
		} break ;
		default: {
			this->status_mask = CHUNK_ERROR;
		} return ;
	}
	this->cursor++;
}

void ChunkContext::consume_chunk_size(std::vector<char> &buffer)
{
	size_t       hex_size;

	hex_size = 0;
	while (hex_size+this->cursor < buffer.size() && isxdigit(buffer[hex_size+this->cursor]))
	{
		this->hex.push_back(buffer[this->cursor+hex_size]);
		hex_size++;
	}
	if (hex_size == 0)	
	{
		this->status_mask = CHUNK_ERROR; // Malformed chunk, expected hex size
		return ;
	}
	this->cursor += hex_size;
	if (this->cursor >= buffer.size())
		return ;
	this->convert_remaining_into_hex();

	if (this->status_mask & CHUNK_ERROR) {
		return ;
	}
	if (this->remaining == 0)
		this->status_mask |= CHUNK_COMPLETE;
	this->status_mask |= CHUNK_TRAILER;
}

void ChunkContext::consume_chunk_data(std::vector<char> &buffer)
{
	size_t to_read;

	to_read = std::min((size_t)(buffer.size() - this->cursor), this->remaining);
	if (to_read == 0)
		return ;
	for (size_t i = this->cursor; i < this->cursor + to_read; i++)
		this->chunk_data.push_back(buffer[i]);
	this->remaining -= to_read;
	this->cursor    += to_read;
	if (this->remaining == 0)
		this->status_mask |= CHUNK_TRAILER;
}

void ChunkContext::unpack(std::vector<char> &buffer)
{
	this->cursor = 0;
	this->chunk_data.clear();
	this->chunk_data.reserve(buffer.size());

	while (this->cursor < buffer.size()
			&& !(this->status_mask & (CHUNK_ERROR | CHUNK_COMPLETE)))
	{
		if (this->is_reading_crlf())
		{
			this->skip_crlf(buffer);
			continue ;
		}
		if (this->is_reading_size()) {
			this->consume_chunk_size(buffer);
			continue ;
		}
		if (this->is_reading_data())
			this->consume_chunk_data(buffer);
	}

	if (this->status_mask & CHUNK_COMPLETE)
		this->status_mask = CHUNK_COMPLETE;
	buffer.swap(this->chunk_data);
}

void ChunkContext::convert_remaining_into_hex()
{
	std::stringstream ss;

	ss << std::hex << this->hex;
	if (!(ss >> this->remaining))
		this->status_mask = CHUNK_ERROR;
	this->hex.clear();
}

bool HttpParser::setChunkedEncoding(void) 
{
	std::pair<bool, std::string> pair = this->getHeaderValue("transfer-encoding");
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
		std::stringstream ss(pair.second);
		if (!(ss >> this->targetBodySize)) {
			this->request.setcode(BadRequest);
		}
		else
			this->request.getHttpRequest().content_length = this->targetBodySize;
		return ;
	}
	this->request.setcode(BadRequest);
}

void HttpParser::handle()
{
	std::string headersOnly;

    if (this->parent == NULL)
        return;
    if (state() == IDLE)
    {
		std::vector<char>::iterator it = utility::search(parent->_buffer, "\r\n\r\n");
        if (it != parent->_buffer.end())
        {
			size_t endOfHeaders = ((it + 4) - parent->_buffer.begin());
            headersOnly = utility::collect(parent->_buffer, endOfHeaders);
            parent->_buffer.erase(parent->_buffer.begin(),
					parent->_buffer.begin() + endOfHeaders);
			this->getRequestObject().setBody(&parent->_buffer);
            this->lexerInstence.tokenize(headersOnly);
            if (lexerInstence.isBadRequest())
            {
                this->request.setcode(BadRequest);
                this->currentState = READY;
                return;
            }
            this->currentState = HEADERS_DONE;
        }
		if (headersOnly.size() > MAX_HEADERS_SIZE
			|| (!headersOnly.size() && (parent->_buffer.size() > MAX_HEADERS_SIZE)))
		{
			this->request.setcode(BadRequest);
			this->currentState = READY;
			return;
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
