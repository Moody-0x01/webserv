#include "HTTP/Response.hpp"
#include <Server.hpp>

Resource::Resource(): __rstream(NULL), __isbuf(false), __done(false), __isopen(false), type(std::string("")), __headers_sent(false)
{
	this->__stream_buffer = "";
	this->bytes_sent = 0;
	this->resource_type = Text;
}

void Resource::identify_type(const std::string &path, const std::map<std::string, std::string> *mime_overrides)
{
    if (path.empty())
    {
		this->setmime_type(ApplicationOctet);
        return ;
    }
    size_t dot = path.find_last_of('.');
    if (dot == std::string::npos || dot == path.size() - 1)
    {
		this->setmime_type(ApplicationOctet);
        return ;
    }
    std::string ext = path.substr(dot + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

	if (mime_overrides)
	{
		std::map<std::string, std::string>::const_iterator over = mime_overrides->find(ext);
		if (over != mime_overrides->end())
		{
			this->setmime_type(over->second);
			return;
		}
	}

    std::map<std::string, std::string>::iterator it = Response::mimes.find(ext);
    if (it != Response::mimes.end())
	{
		this->setmime_type(it->second);
		return ;
	}
	this->setmime_type(ApplicationOctet);
}

void Resource::setmime_type(std::string t)
{
	this->type = t;
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
	}
	Resource::identify_type(path);
	this->__isopen = true;
	this->resource_type = File;
	return (OK);
}

Resource::~Resource()
{
	if (this->__rstream) {
		delete (this->__rstream);
	}
}

bool Resource::isdone(void)
{
	return (this->__done);
}

bool Resource::isopen(void)
{
	return (this->__isopen);
}

bool Resource::headers_sent(void)
{
	return (this->__headers_sent);
}

void Resource::set_headers_sent(void)
{
	this->__headers_sent = true;
}

response_stage_t Resource::send(const HttpRequest &request) __THROWS_STRERROR
{
	char buffer[SENDING_CHUNK_SIZE];

	if (this->__stream_buffer.empty())
	{
		if (this->__isbuf)
		{
			this->__done = true;
			return (DoneSending);
		}
		if (this->resource_type != File || !this->__rstream || !this->__rstream->is_open())
		{
			this->__done = true;
			return (DoneSending);
		}
		this->__rstream->read(buffer, sizeof(buffer));
		std::streamsize bytes_read = this->__rstream->gcount();
		if (bytes_read <= 0)
		{
			if (this->__rstream->bad())
				throw "failed reading resource file";
			this->__done = true;
			return (DoneSending);
		}
		this->__stream_buffer.assign(buffer, buffer + bytes_read);
		this->bytes_sent = 0;
	}

	ssize_t sent = ::write(request.conn,
		this->__stream_buffer.data() + this->bytes_sent,
		this->__stream_buffer.size() - this->bytes_sent);
	if (sent <= 0)
		throw "client disconnected while sending response";
	this->bytes_sent += static_cast<size_t>(sent);
	if (this->bytes_sent < this->__stream_buffer.size())
		return (SendingResource);
	this->__stream_buffer.clear();
	this->bytes_sent = 0;
	if (this->__isbuf)
	{
		this->__done = true;
		return (DoneSending);
	}
	return (SendingResource);
}

std::string Resource::getmime_type(void)
{
	return (this->type);
}

void Resource::set_stream_buffer(const std::string &s)
{
	this->__isbuf = true;
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
