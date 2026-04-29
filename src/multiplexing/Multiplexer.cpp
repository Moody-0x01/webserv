#include <Server.hpp>
#include <cstdlib>
#include <strings.h>
#include <sys/epoll.h>
#include <sys/socket.h>


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
}

void Multiplexer::abort(void)
{
	this->aborted = true;
}

void Multiplexer::init(std::vector<ServerConfig> &confs) throw(std::runtime_error, const char *)
{
	size_t alive;

	this->aborted = false;
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
	this->signal_handler.init();
	this->signal_handler.epoll_register(this->epoll_fd);
	this->signal_handler.setup_signal_handlers();
}

static std::string resolve_host(const std::string &host)
{
    if (host.empty()) return "0.0.0.0";
    if (host == "localhost") return "127.0.0.1";
    return host;
}

void Multiplexer::register_server(ServerConfig &conf) __THROWS_STRERROR
{
	int server_fd, opt, code;
	Multiplexer *self;
	struct epoll_event event;
	struct addrinfo hints, *res;

	self = Multiplexer::get_multiplexer(NULL);
	conf.host = resolve_host(conf.host);
	std::memset(&hints, 0, sizeof(hints));
    hints.ai_family   = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    opt = 1;
	code = getaddrinfo(conf.host.c_str(), conf.port.c_str(), &hints, &res);
	if (code != 0) throw gai_strerror(code);
	server_fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if (server_fd < 0) throw strerror(errno);
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    std::memset(&event, 0, sizeof(event));
    if (bind(server_fd, res->ai_addr, res->ai_addrlen) < 0)
    {
        freeaddrinfo(res);
		throw strerror(errno);
    }
    freeaddrinfo(res);
	if (set_nonblocking(server_fd) == -1)
		throw strerror(errno);
    if (listen(server_fd, SOMAXCONN) < 0)
		throw strerror(errno);

	self->servers[server_fd] = std::make_pair(Server(), Clients());
	self->confs[server_fd]   = conf;
	self->servers[server_fd].first.set_socket(server_fd);
	event.events = EPOLLIN | EPOLLERR | EPOLLHUP | EPOLLRDHUP;
	event.data.ptr = &self->servers[server_fd].first;
	code = epoll_ctl(self->epoll_fd, EPOLL_CTL_ADD, server_fd, &event);
	if (code < 0)
	{
		self->servers
			.erase(server_fd);
		throw strerror(errno);
	}
	Multiplexer::introduce_new_context((uint64_t)&self->servers[server_fd].first);
}

Client *Multiplexer::register_client(uint32_t e, Server *server) __THROWS_STRERROR
{
	struct sockaddr_in addr;
	uint32_t ip;
	Multiplexer *self;
	int conn;
	socklen_t len;
	(void)e;

	self = Multiplexer::get_multiplexer(NULL);

	len = sizeof(addr);
	conn = accept(server->get_socket(), (struct sockaddr*)&addr, &len);
	if (conn == -1) throw strerror(errno);
	ip = ntohl(addr.sin_addr.s_addr);
	if (set_nonblocking(conn) == -1) throw strerror(errno);

	self->servers[server->get_socket()].second[conn] = Client();

	Client &client = self->servers[server->get_socket()].second[conn];

	client.setip_from_bytes(ip);
	client.set_owner(server->get_socket());
	client.set_socket(conn);
	client.getParser().getRequestObject().set_sockets(server->get_socket(), conn);
	client.getParser().setParent(&client);
	return (&client);
}

void Multiplexer::unregister_client(int owner, int client) __THROWS_STRERROR
{

	Multiplexer *self;

	self = Multiplexer::get_multiplexer(NULL);
	self->servers[owner].second
		.erase(client);
}

void unregister_fd(int fd)
{
	Multiplexer *self;
	epoll_event dummy;

	if (fd >= 0) {
		self = Multiplexer::get_multiplexer(NULL);
		epoll_ctl(self->epoll_fd, EPOLL_CTL_DEL, fd, &dummy);
		close(fd);
	}
}

Multiplexer::~Multiplexer()
{
	this->deinit();
}

void Multiplexer::deinit(void)
{
	close(this->epoll_fd);
}

int Multiplexer::run(void)
{
	Multiplexer::introduce_new_context((uint64_t)&this->signal_handler);
    while (this->servers.size() && !this->aborted)
	{
		int ready = epoll_wait(this->epoll_fd,
						 this->events, EVENT_MAX, 100);
		if (ready < 0)
		{
			if (errno == EINTR) continue;
			std::cerr << "[ Multiplexer::loop ] epoll_wait: " << strerror(errno) << "\n";
			return 1;
		}
		for (int index = 0; index < ready && (this->servers.size()) && !this->aborted; ++index)
			this->execute_epoll_event(index);
    }
	return (this->aborted ? 0 : 1);
}

void Multiplexer::execute_epoll_event(int epoll_index)
{
	void *ptr;

	ptr = this->events[epoll_index].data.ptr;
	if (!this->iscontext_valid((uint64_t)ptr))
		return ;
	ASocketContext *handle = (ASocketContext *)ptr;
	try {
		handle->action(this->events[epoll_index].events);
	} catch (const char *error) {
		std::cerr << "[ handle->action ] " << error << "\n";
	}
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

void Multiplexer::introduce_new_context(uint64_t context)
{
	Multiplexer *self;

	self = Multiplexer::get_multiplexer(NULL);
	self->_valid_context.insert(context);
}

void   Multiplexer::unintroduce_context(uint64_t context)
{
	Multiplexer *self;

	self = Multiplexer::get_multiplexer(NULL);
	self->_valid_context.erase(context);
}

bool   Multiplexer::iscontext_valid(uint64_t context)
{
	Multiplexer *self;

	self = Multiplexer::get_multiplexer(NULL);
	return (self->_valid_context.find(context) != self->_valid_context.end());
}

void Multiplexer::kill(void)
{
	Multiplexer *self;

	self = Multiplexer::get_multiplexer(NULL);
	self->abort();
}
