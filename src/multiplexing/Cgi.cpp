#include <Server.hpp>

void Cgi::write() __THROWS_STRERROR
{
}

void Cgi::read() __THROWS_STRERROR
{
}

void Cgi::action(uint32_t e) __THROWS_STRERROR
{
	if (e & (EPOLLERR | EPOLLHUP))
    {
        /*  this->free();  */
		// Note: in case of an error in the cgi, just cleanup resources here.
		// unregister from epoll whatever was registerred. then stop.
		return ;
    }

	try {
    if (e & EPOLLIN)
	{
		// TODO: Read from the process and hand to the client somewhere else.
		this->read();
	}
    if ((e & EPOLLOUT) || (e & EPOLLRDHUP))
	{
		// TODO: Read the body from the client then hand it to the script.
		this->write();
	}
	} catch (const char *e) {
		throw e;
	}
}

Cgi::Cgi()
{
	for (size_t i = 0; environ[i]; ++i) this->env.push_back(environ[i]);
	this->start_time          = 0;
	this->content_length      = 0;
	this->bytes_read_from_cgi = 0;
}
