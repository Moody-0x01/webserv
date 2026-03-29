#include <Server.hpp>
#include <cerrno>
#include <cstdio>
#include <fstream>
#include <string>

Resource::Resource(): __rstream(NULL), __done(false), __isopen(false), type(std::string(""))
{
	this->__stream_buffer = "";
	this->bytes_sent = 0;
}

void Resource::identify_type(const std::string &path)
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
    std::map<std::string, std::string>::iterator it = Response::mimes.find(ext);
    if (it != Response::mimes.end())
        this->type = it->second;
    else
        this->type = ApplicationOctet;
}

int Resource::open(const std::string &path) __THROWS_STRERROR
{
	(void)(path);
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

void Resource::sendchunk(int who) __THROWS_STRERROR
{
	// TODO: make type of the Resource..
	(void)who;
}


std::string Resource::get_type(void)
{
	return (this->type);
}

void Resource::set_stream_buffer(const std::string &s)
{
	this->__stream_buffer = s;
}
