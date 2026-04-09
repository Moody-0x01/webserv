#include <Server.hpp>

int set_nonblocking(int sockfd)
{
	errno = 0;
    int flags = fcntl(sockfd, F_GETFL);
    if (flags == -1) return -1;
	flags |= O_NONBLOCK;
    if (fcntl(sockfd, F_SETFL, flags) == -1) return -1;
    return 0;
}

std::string get_signal_name(int sig)
{
    static std::map<int, std::string> sig_map;

    sig_map[SIGINT]  =  "SIGINT (Interrupt)";
    sig_map[SIGTERM] =  "SIGTERM (Termination)";
    sig_map[SIGHUP]  =  "SIGHUP (Hangup/Reload)";
    sig_map[SIGUSR1] =  "SIGUSR1 (User Defined 1)";
    sig_map[SIGUSR2] =  "SIGUSR2 (User Defined 2)";
    sig_map[SIGQUIT] =  "SIGQUIT (Quit/Core Dump)";
    if (sig_map.count(sig)) return sig_map[sig];
    return "Unknown Signal";
}

void signal_handler(int sig)
{
	unsigned char* data;
    int saved_errno = errno;
	Multiplexer *self;

	self = Multiplexer::get_multiplexer(NULL);
	if (!self) throw "Well, failed to get a Multiplexer class";

	data = (unsigned char*)&sig;

    write(self->signal_io[1],
		data,
		sizeof(int));
    errno = saved_errno;
}

Multiplexer *Multiplexer::get_multiplexer(std::vector<ServerConfig> *confs) throw(std::runtime_error, const char *)
{
	static Multiplexer m;
	if (confs) m.init(*confs);
	return (&m);
}

Multiplexer *Multiplexer::create_multiplexer(std::vector<ServerConfig> &confs)
{
	return (Multiplexer::get_multiplexer(&confs));
}

const ServerConfig &Multiplexer::get_conf(int fd)
{
	Multiplexer *self;

	self = Multiplexer::get_multiplexer(NULL);
	return (self->confs[fd]);
}

Server *Multiplexer::get_owner(int fd) __THROWS_STRERROR
{
	if (this->servers.find(fd) == this->servers.end())
		throw "Client did not find its owner";
	return (&this->servers[fd].first);
}

Multiplexer::Multiplexer() __THROWS_STRERROR: epoll_fd(epoll_create(IGNORED))
{
	if (this->epoll_fd < 0)
		throw std::strerror(errno);
	Response::init_status_lines();
	Response::init_mimes();	
	try {
		this->init_signals();
	} catch (const char *e) {
		throw e;
	}
	this->signal_io[STDOUT_FILENO] = -1;
	this->signal_io[STDIN_FILENO ] = -1;
}

void Multiplexer::init(std::vector<ServerConfig> &confs) throw(std::runtime_error, const char *)
{
	size_t alive;

	alive = 0;
	for (size_t c = 0; c < confs.size(); c++)
	{
		try {
			this->register_server(confs[c]);
			alive++;
		} catch (const char *error) {
			std::cerr << "[ Multiplexer::register_server ] " << error << "\n";
		}
	}
	if (alive > 0) return ;
	throw std::runtime_error("There are no hosts to continue further.");
}

void Multiplexer::init_signals(void) __THROWS_STRERROR
{
    struct epoll_event event;
    int code;

    if (pipe(this->signal_io) < 0)			 throw strerror(errno);
    if (set_nonblocking(this->signal_io[0])) throw strerror(errno);
    if (set_nonblocking(this->signal_io[1])) throw strerror(errno);

    event.events = EPOLLIN;
    event.data.ptr = this->signal_io; 
    code = epoll_ctl(this->epoll_fd, EPOLL_CTL_ADD, 
                    this->signal_io[0], &event);
    if (code < 0) throw strerror(errno);
    if (signal(SIGPIPE, SIG_IGN) == SIG_ERR)         throw strerror(errno);
    if (signal(SIGINT,  signal_handler) == SIG_ERR)  throw strerror(errno);
    if (signal(SIGTERM, signal_handler) == SIG_ERR)  throw strerror(errno);
    if (signal(SIGHUP,  signal_handler) == SIG_ERR)  throw strerror(errno);
    if (signal(SIGQUIT,  signal_handler) == SIG_ERR) throw strerror(errno);
	if (signal(SIGCHLD, signal_handler) == SIG_ERR)  throw strerror(errno);	
}

static std::string resolve_host(const std::string &host)
{
    if (host.empty()) return "0.0.0.0";
    if (host == "localhost") return "127.0.0.1";
    return host;
}

void Multiplexer::register_server(ServerConfig &conf) __THROWS_STRERROR
{
	Server server;
	int server_fd, opt, code;
	Multiplexer *self;
	struct epoll_event event;
	struct addrinfo hints, *res;

	self = Multiplexer::get_multiplexer(NULL);
	if (!self) throw "Well, failed to get a Multiplexer class";
	conf.host = resolve_host(conf.host);
	std::memset(&hints, 0, sizeof(hints));
    hints.ai_family   = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    opt = 1;
	code = getaddrinfo(conf.host.c_str(), conf.port.c_str(), &hints, &res);
	if (code != 0) throw gai_strerror(code);
	server_fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if (server_fd < 0) throw strerror(errno);
	server.set_socket(server_fd);
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    std::memset(&event, 0, sizeof(event));
    if (bind(server_fd, res->ai_addr, res->ai_addrlen) < 0)
    {
        freeaddrinfo(res);
		throw strerror(errno);
    }
    freeaddrinfo(res);
	if (set_nonblocking(server.get_socket()) == -1)
		throw strerror(errno);
    if (listen(server.get_socket(), SOMAXCONN) < 0)
		throw strerror(errno);
	server.disown();
	self->servers[server_fd] = std::make_pair(server, Clients());
	self->confs[server_fd] = conf;

	event.events = EPOLLIN;
	event.data.ptr = &self->servers[server_fd].first;
	code = epoll_ctl(self->epoll_fd, EPOLL_CTL_ADD, server.get_socket(), &event);
	if (code < 0)
	{
		self->servers.erase(server_fd);
		throw strerror(errno);
	}
}

Client *Multiplexer::register_client(uint32_t e, Server *server) __THROWS_STRERROR
{
	struct sockaddr_in addr;
	Multiplexer *self;
	int conn;
	socklen_t len;
	Client client;
	(void)e;

	self = Multiplexer::get_multiplexer(NULL);
	if (!self) throw "Well, failed to get a Multiplexer class";

	len = sizeof(addr);
	conn = accept(server->get_socket(), (struct sockaddr*)&addr, &len);
	if (conn == -1) throw strerror(errno);

	client.set_owner(server->get_socket());
	client.set_socket(conn);
	client.getParser().getRequestObject().set_sockets(server->get_socket(), conn);
	if (set_nonblocking(conn) == -1) throw strerror(errno);
	self->servers[server->get_socket()].second[conn]
		.take_ownership(&client);
	return (&self->servers[server->get_socket()].second[conn]);
}


void Multiplexer::unregister_client(int owner, int client) __THROWS_STRERROR
{

	Multiplexer *self;
	self = Multiplexer::get_multiplexer(NULL);
	if (!self) throw "Well, failed to get a Multiplexer class";
	self->servers[owner].second.erase(client);
}

Multiplexer::~Multiplexer()
{
	this->deinit();
}

void Multiplexer::deinit(void)
{
	close(this->epoll_fd);
	if (this->signal_io[0] != -1) close(this->signal_io[0]);
	if (this->signal_io[1] != -1) close(this->signal_io[1]);
	// TODO: Cleanup other stuff..
}

int Multiplexer::loop(void)
{
	int sig;

    while (true)
	{
		int ready = epoll_wait(this->epoll_fd,
						 this->events, EVENT_MAX, 100);
		if (ready < 0)
		{
			if (errno == EINTR) continue;
			std::cerr << "[ Multiplexer::loop ] epoll_wait: " << strerror(errno) << "\n";
			return 1;
		}
		for (int index = 0; index < ready; ++index)
		{
			void *ptr = this->events[index].data.ptr;
			if (ptr == this->signal_io)
			{
				(void)read(this->signal_io[0], &sig, sizeof(sig));
				if (sig != SIGCHLD)
				{
					std::cerr << "[ Multiplexer::loop ] Encountered " << get_signal_name(sig) << "\n";
					return 1;
				}
				while (waitpid(-1, NULL, WNOHANG) > 0);
			}
			else {
				ASocketContext *handle = (ASocketContext *)ptr;
				try {
					handle->action(this->events[index].events);
				} catch (const char *error) {
					std::cerr << "[ handle->action ] " << error << "\n";
				}
			}
		}
    }
	return (0);
}

ssize_t Multiplexer::read(int fd, void *buf, size_t size) __THROWS_STRERROR
{
	ssize_t count = ::read(fd, buf, size);
	if (count <= 0) throw strerror(errno);
	return (count);
}

ssize_t Multiplexer::write(int fd, const void *buf, size_t size) __THROWS_STRERROR
{
	ssize_t count = ::write(fd, buf, size);
	if (count <= 0) throw strerror(errno);
	return (count);
}
