#pragma once
#include <string>
#include <map>
#include <sys/types.h>
#include <HTTP/Request.hpp>

struct LocationConfig;

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
#define  TextHtml             "text/html"
#define  TextPlain            "text/plain"
#define  TextCss              "text/css"
#define  TextJavascript       "text/javascript"
#define  TextXml              "text/xml"
#define  TextCsv              "text/csv"

// Images.
#define  ImageJpeg            "image/jpeg"
#define  ImagePng             "image/png"
#define  ImageGif             "image/gif"
#define  ImageWebp            "image/webp"
#define  ImageSvg             "image/svg+xml"
#define  ImageIco             "image/x-icon"

// Other
#define  ApplicationJson      "application/json"
#define  ApplicationXml       "application/xml"
#define  ApplicationPdf       "application/pdf"
#define  ApplicationZip       "application/zip"
#define  ApplicationOctet     "application/octet-stream"
#define  ApplicationForm      "application/x-www-form-urlencoded"
#define  ApplicationJs        "application/javascript"


// Other audio
#define  AudioMpeg            "audio/mpeg"
#define  AudioOgg             "audio/ogg"
#define  AudioMp3 "audio/mp3"

// Video
#define  VideoMp4             "video/mp4"
#define  VideoWebm            "video/webm"

typedef enum resource_type_e
{
	File,
	Text,
	Dir,
	Cgi
} resource_type_t;

class Resource
{

	resource_type_t  resource_type;
	std::ifstream *__rstream;
	std::string __stream_buffer;
	bool         __done;
	bool         __isopen;
	// char         buffer[WRITE_CHUNK_SIZE]; Well be used to send chuncks
	size_t       bytes_sent;
	std::string  type;


	public:
		Resource();

		void identify_type(const std::string &path, const std::map<std::string, std::string> *mime_overrides = NULL);
		int open(const std::string &path); // TODO: Init the Resource, 
			// identify the mime type. if it is supported, if not then BadRequest error page should be set up and sent
		~Resource();
		bool isdone(void);
		bool isopen(void);
		std::string get_type(void);
		void send(const HttpRequest &request) __THROWS_STRERROR;
		void set_stream_buffer(const std::string &s);
		const std::string &get_stream_buffer(void) const;
		resource_type_t getresource_type(void) const;
		void setresource_type(resource_type_t t);
};

typedef enum response_stage_e {
	Setup,
	SendingResource,
} response_stage_t;

struct UriResolutionResult {
	enum type {
		None,
		directory,
		cgi,
		file
	};
	UriResolutionResult();
	type						resource_type;
	const LocationConfig	*matched_location;
	std::string			request_path;
	std::string			root;
	std::string			index;
	std::string			filesystem_path;
	std::pair <std::string, std::string> cgi_script; // holds full script path and interpreter, empty if not a cgi
};

class Response
{
private:
	enum MethodKind
	{
		MethodGet,
		MethodPost,
		MethodDelete,
		MethodInvalid
	};

	static MethodKind classify_method(const std::string &method);
	void handle_get(const HttpRequest &request);
	void handle_post(const HttpRequest &request);
	void handle_delete(const HttpRequest &request);

	void list_dir(void);
	void serve_file(void);

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

	Resource             resource; // NOTE: response if the request has to be responded by some file. *.html, *.mp3, *.mp4, error page? idk
	UriResolutionResult  resolved_results; // holds the resultion struct
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
	void appendheader(const char *key, const char  *value);
	void set_status(int s);
	int  get_status(void) const;
	const UriResolutionResult &get_resolved_results(void) const;
	static UriResolutionResult resolve_uri_to_path(const HttpRequest &request);
	const std::string get_error_page_html(const HttpRequest &request, int code);
	void setup_response(const HttpRequest &request);
	void continue_processing(const HttpRequest &request);
	bool is_method_allowed(std::string method);
	response_stage_t getstage(void) const;
};
