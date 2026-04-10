#include "HTTP/Response.hpp"
#include <Server.hpp>
#include <algorithm>
#include <cerrno>
#include <cstddef>
#include <cstdio>
#include <fstream>
#include <string>

Resource::Resource(): __rstream(NULL), __done(false), __isopen(false), type(std::string(""))
{
	this->__stream_buffer = "";
	this->bytes_sent = 0;
	this->resource_type = Text;
}

void Resource::identify_type(const std::string &path, const std::map<std::string, std::string> *mime_overrides)
{
    if (path.empty())
    {
        this->type = ApplicationOctet;
        return ;
    }
    size_t dot = path.find_last_of('.');
    if (dot == std::string::npos || dot == path.size() - 1)
    {
        this->type = ApplicationOctet;
        return ;
    }
    std::string ext = path.substr(dot + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

	if (mime_overrides)
	{
		std::map<std::string, std::string>::const_iterator over = mime_overrides->find(ext);
		if (over != mime_overrides->end())
		{
			this->type = over->second;
			return;
		}
	}

    std::map<std::string, std::string>::iterator it = Response::mimes.find(ext);
    if (it != Response::mimes.end())
        this->type = it->second;
    else
        this->type = ApplicationOctet;
}

int Resource::open(const std::string &path)
{
	errno = 0;

	this->__rstream = new std::ifstream(path.c_str());
	if (!this->__rstream->is_open())
	{
		delete this->__rstream;
		this->__rstream = NULL;
		if (errno == ENOENT) return (NotFound);
        if (errno == EACCES) return (Unauthorized);
        return (InternalServerError);
		// Note: returning from here means that something did not happen as expected and it we need to actually handle the error and return the server code.
	}
	// TODO: Set the mime type which is the type here
	// TODO: if anything break while opening then 500 should be thrown to the user and the serer should log whateer happened.
	// TODO: set __isopen to true
	// this->__filename = path; // NOTE: Maybe we need it for logging errors?
	Resource::identify_type(path);
	this->__isopen = true;
	this->resource_type = File;
	// Note: returning from here means everything was okay. u may send the body as a file
	return (OK);
}

Resource::~Resource()
{
	if (this->__rstream) delete (this->__rstream);
}

bool Resource::isdone(void)
{
	return (this->__done);
}

bool Resource::isopen(void)
{
	return (this->__isopen);
}
#include <cmath>
void Resource::send(const HttpRequest &request) __THROWS_STRERROR
{
	switch (this->resource_type)
	{
		case File: {
			::write(request.conn, "Sending a static file", 22);
		} break;
		case Dir:
		case Text: {
			this->__stream_buffer = "Sending a static text";
			if (!request.query_string.empty())
			{
				this->__stream_buffer.append(",  query string: ");
				this->__stream_buffer.append(request.query_string.c_str());
			}
			if (this->bytes_sent < this->__stream_buffer.size()) {
				// size_t n = min((int)WRITE_CHUNK_SIZE, (int)this->bytes_sent -this->__stream_buffer.size());
				this->bytes_sent += ::write(request.conn, 
					this->__stream_buffer.c_str(), 
					this->__stream_buffer.size());
			} else
				this->__done = true;
		} break;
		case Cgi: {
			::write(request.conn, "Sending Cgi", 12);
		} break;
		default: 
			assert(0 && "Bro wtf??");
	}
}

std::string Resource::get_type(void)
{
	return (this->type);
}

void Resource::set_stream_buffer(const std::string &s)
{
	this->__stream_buffer = s;
}

const std::string &Resource::get_stream_buffer(void) const
{
	return this->__stream_buffer;
}

resource_type_t Resource::getresource_type(void) const
{
	return (this->resource_type);
}

void Resource::setresource_type(resource_type_t t)
{
this->resource_type = t;
}
