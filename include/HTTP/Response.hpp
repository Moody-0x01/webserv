#pragma once
#include <string>
#include <map>
#include <sys/types.h>

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
	// int    __resource_fd;
	// bool   __done;
	// size_t sent;
	// char   buffer[WRITE_CHUNK_SIZE];
	// char  *type;


	public:
		Resource();
		Resource(const char *path); // TODO: Init the Resource, 
			// identify the mime type. if it is supported, if not then BadRequest error page should be set up and sent
		~Resource();
		bool isdone(void);
		void sendchunk() __THROWS_STRERROR;
};

class Response
{
private:
	// Note: well, a Response should most probably have a write method???  No??
    std::map<std::string, std::string> headers;
    std::string body;
	std::string __serialized_response; // NOTE: builtup response. from headers and body into one full response.
	bool        __is_serialized;
	size_t      sent;
	Resource    resource; // NOTE: response if the request has to be responded by some file. *.html, *.mp3, *.mp4, error page? idk

public:
    Response();
    ~Response();

	static std::map<int, std::string> status_lines;
	static void init_status_lines();
	void write(int conn) __THROWS_STRERROR; // NOTE: writes the wrapped response into the the client connexion
	bool isdone();
	void serialize(void);
};
