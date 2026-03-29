#pragma once
#include <string>
#include <map>
#include <sys/types.h>
#include <HTTP/Request.hpp>

# define __THROWS_STRERROR throw(const char *)

#define  WRITE_CHUNK_SIZE     4096 // 4kb each time.

#define  OK                   200
#define  Created              201
#define  NoContent            204
#define  MovedPermanently     301
#define  Found                302
#define  NotModified          304
#define  BadRequest           400
#define  Unauthorized         401
#define  Forbidden            403
#define  NotFound             404
#define  MethodNotAllowed     405
#define  RequestTimeout       408
#define  InternalServerError  500
#define  NotImplemented       501
#define  BadGateway           502
#define  ServiceUnavailable   503

// Mime types
#define  TextHtml             "text/html\r\n"
#define  TextPlain            "text/plain\r\n"
#define  TextCss              "text/css\r\n"
#define  TextJavascript       "text/javascript\r\n"
#define  TextXml              "text/xml\r\n"
#define  TextCsv              "text/csv\r\n"

// Images.
#define  ImageJpeg            "image/jpeg\r\n"
#define  ImagePng             "image/png\r\n"
#define  ImageGif             "image/gif\r\n"
#define  ImageWebp            "image/webp\r\n"
#define  ImageSvg             "image/svg+xml\r\n"
#define  ImageIco             "image/x-icon\r\n"

// Other
#define  ApplicationJson      "application/json\r\n"
#define  ApplicationXml       "application/xml\r\n"
#define  ApplicationPdf       "application/pdf\r\n"
#define  ApplicationZip       "application/zip\r\n"
#define  ApplicationOctet     "application/octet-stream\r\n"
#define  ApplicationForm      "application/x-www-form-urlencoded\r\n"
#define  ApplicationJs        "application/javascript\r\n"


// Other audio
#define  AudioMpeg            "audio/mpeg\r\n"
#define  AudioOgg             "audio/ogg\r\n"
#define  AudioMp3 "audio/mp3"

// Video
#define  VideoMp4             "video/mp4\r\n"
#define  VideoWebm            "video/webm\r\n"

class Resource
{
	// resource_type_t 
	std::ifstream *__rstream;
	std::string __stream_buffer;
	size_t       bytes_sent;
	bool         __done;
	bool         __isopen;
	// char         buffer[WRITE_CHUNK_SIZE]; Well be used to send chuncks
	std::string  type;


	public:
		Resource();

		void identify_type(const std::string &path);
		int open(const std::string &path) __THROWS_STRERROR; // TODO: Init the Resource, 
			// identify the mime type. if it is supported, if not then BadRequest error page should be set up and sent
		~Resource();
		bool isdone(void);
		bool isopen(void);
		std::string get_type(void);
		void sendchunk(int who) __THROWS_STRERROR; // Sends the next chunck to `who`
		void set_stream_buffer(const std::string &s);
};

typedef enum response_stage_e {
	Setup = 0x0,
	SendingHeaders,
	SendingFile,
	// SendingCgi,
} response_stage_t;

class Response
{
private:
	// Note: well, a Response should most probably have a write method???  No??
	// Note: I should most probably make methods for serializing the response headers, then the body...
	// Once headers weere serialized and sent. then the state should be switched to sending the body... in that case 
	int status;
	// Will be generated last after headers and opening the file resource
	std::string status_line; // HTTP/1.0 Code Message
	// isfile?

	std::map<std::string, std::string> headers;
	response_stage_t stage;
	std::string headers_as_str;
	int bytes_sent;

	Resource    resource; // NOTE: response if the request has to be responded by some file. *.html, *.mp3, *.mp4, error page? idk
public:
	static std::map<std::string, std::string> mimes;
	static void init_mimes();
    Response();
    ~Response();

	static std::map<int, std::string> status_lines;
	static void init_status_lines();

	void write(int conn) __THROWS_STRERROR; // NOTE: writes the wrapped response into the the client connexion
	void send_headers(void) __THROWS_STRERROR;
	bool isdone();
	void serialize_headers(void);
	void appendheader(const std::string key, const std::string value);
	void set_status(int s);
	int  get_status(void) const;
    void setup_response(const HttpRequest &request);
	void continue_processing(const HttpRequest &request);
	response_stage_t getstage(void) const;
};
