<<<<<<< HEAD
#include <Server.hpp>



int main()
{
    HttpParser http;
=======
# include <Server.hpp>

int main() {	 
	Multiplexer::init();
	Multiplexer::loop();
	Multiplexer::deinit();
>>>>>>> origin/master
    return 0;
}
