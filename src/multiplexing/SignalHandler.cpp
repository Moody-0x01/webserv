# include <Server.hpp>

SignalHandler::~SignalHandler(void)
{
	ASocketContext::~ASocketContext();
	close(this->io[0]);
	close(this->io[1]);
}

void SignalHandler::action(uint32_t e) __THROWS_STRERROR {
	if (e & EPOLLIN)
	{
		int signum = this->read_signal();
		if (signum == SIGINT || signum == SIGTERM || signum == SIGHUP || signum == SIGQUIT) {
			std::cout << "Received signal " << strsignal(signum) << ", shutting down...\n";
			Multiplexer::kill();
		}
	}
}

void SignalHandler::set_signalio(int io[2])
{
	this->io[0] = io[0];
	this->io[1] = io[1];
}

void SignalHandler::init(void) __THROWS_STRERROR
{
    if (pipe(this->io) < 0)			 throw strerror(errno);
    if (set_nonblocking(this->io[0])) throw strerror(errno);
    if (set_nonblocking(this->io[1])) throw strerror(errno);
}


void SignalHandler::epoll_register(int epoll_fd) __THROWS_STRERROR
{
    int code;
    struct epoll_event event;

    event.events = EPOLLIN;
    event.data.ptr = this; 
    code = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, this->io[0], &event);
    if (code < 0) throw strerror(errno);
}

void SignalHandler::setup_signal_handlers() __THROWS_STRERROR {
    if (signal(SIGPIPE, SIG_IGN) == SIG_ERR)          throw strerror(errno);
    if (signal(SIGINT,  signal_handler) == SIG_ERR)   throw strerror(errno);
    if (signal(SIGTERM, signal_handler) == SIG_ERR)   throw strerror(errno);
    if (signal(SIGHUP,  signal_handler) == SIG_ERR)   throw strerror(errno);
    if (signal(SIGQUIT, signal_handler) == SIG_ERR)   throw strerror(errno);
	if (signal(SIGCHLD, sigpipe_handler) == SIG_ERR)  throw strerror(errno);
}

void SignalHandler::write_signal(int signum) __THROWS_STRERROR
{
	if (write(this->io[1], &signum, sizeof(signum)) < 0) throw strerror(errno);
}

int SignalHandler::read_signal(void) __THROWS_STRERROR
{
	int signum;

	ssize_t bytes_read = read(this->io[0], &signum, sizeof(signum));
	if (bytes_read > 0) {
		return signum;
	} else if (bytes_read == -1) {
		throw strerror(errno);
	}
	return -1; // No signal read
}
