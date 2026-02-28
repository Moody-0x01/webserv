# include <Server.hpp>

int main() {	 
	Multiplexer::init();
	Multiplexer::loop();
	Multiplexer::deinit();
    return 0;
}
