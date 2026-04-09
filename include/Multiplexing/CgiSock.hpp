# include <Multiplexing/ASocketContext.hpp>

typedef enum cgi_state_e {
	WRITING_BODY,
	READING_RESPONSE
} cgi_state_t;

typedef struct Cgi_: public ASocketContext {
public:
	void action(uint32_t e) __THROWS_STRERROR;
	pid_t       pid;
	int         streams[2];
	cgi_state_t state;
} Cgi_;
