#include <Server.hpp>


Resource::Resource()
{
}
Resource::Resource(const char *path)
{
	// TODO: Init the Resource, 
	// identify the mime type. if it is supported, if not then BadRequest error page should be set up and sent
	(void) path;
}

Resource::~Resource()
{
}

bool Resource::isdone(void)
{
	return true;
}

void Resource::sendchunk() __THROWS_STRERROR
{
}
