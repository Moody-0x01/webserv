#include "HTTP/Response.hpp"
#include <Server.hpp>
#include <cstdlib>
#include <map>
#include <string>
#include <utility>

std::map<int, std::string> Response::status_lines;
std::map<std::string, std::string> Response::mimes;

// static bool is_methodvalid(const std::string &method)
// {
// 	return ((method == "GET") || (method == "POST") || (method == "DELETE"));
// }

void Response::init_mimes()
{
	if (!mimes.empty()) return ;
	Response::mimes["html"]  =  TextHtml          ;
	Response::mimes["txt"]   =  TextPlain         ;
	Response::mimes["css"]   =  TextCss           ;
	Response::mimes["js"]    =  TextJavascript    ;
	Response::mimes["xml"]   =  TextXml           ;
	Response::mimes["csv"]   =  TextCsv           ;
	Response::mimes["jpg"]   =  ImageJpeg         ;
	Response::mimes["png"]   =  ImagePng          ;
	Response::mimes["gif"]   =  ImageGif          ;
	Response::mimes["webp"]  =  ImageWebp         ;
	Response::mimes["svg"]   =  ImageSvg          ;
	Response::mimes["ico"]   =  ImageIco          ;
	Response::mimes["json"]  =  ApplicationJson   ;
	Response::mimes["xml"]   =  ApplicationXml    ;
	Response::mimes["pdf"]   =  ApplicationPdf    ;
	Response::mimes["zip"]   =  ApplicationZip    ;
	Response::mimes["bin"]   =  ApplicationOctet  ;
	Response::mimes["form"]  =  ApplicationForm   ;
	Response::mimes["js"]    =  ApplicationJs     ;
	Response::mimes["mp3"]   =  AudioMpeg         ;
	Response::mimes["ogg"]   =  AudioOgg          ;
	Response::mimes["mp3"]   =  AudioMp3          ;
	Response::mimes["mp4"]   =  VideoMp4          ;
	Response::mimes["webm"]  =  VideoWebm         ;
}

Response::Response()
{
	this->stage = Setup;
}

void Response::continue_processing(const HttpRequest &request)
{
	
	// ServerConfig c = Multiplexer::confs[request.owner];

	if (this->stage == Setup) this->setup_response(request); // NOTE: a call to set_status(code) is mandatory before sending headers.
	this->stage = SendingHeaders;
	switch (this->stage)
	{
		case Setup: 
		case SendingHeaders: {
			// setup_response: setup these
			// 1) Resources that will be sent.
			// 2) Headers that will be send
			// 3) Status lines that will be send based off the availability if the requested stuff.
			if (!this->headers_as_str.size()) // Not serialized yet it should be serialized first 
				this->serialize_headers();
			this->send_headers();
		} break;
		case SendingResource: {
			// if (this->resource.getresource_type() == Cgi)
			// 	// execute: Params, bin
			// if (this->requested.getresource_type() == File)
			// this->resource.send_resource();
			// NOTE: send_resource:
	
			// If (response != OK)
			//      // send_error()
			// If (req == cgi)
			//      // Take paramas.
			//		// execute bin with params
			//		// send..
			// If (req == File)
			//      // send the file that was opened in the setup
			// If (req == Text)
			//      // Send the text

		} break;
		// case SendingCgi: {} break;
		default:
			abort();
	}
}

void Response::setup_response(const HttpRequest &request)
{
	// TODO: Validate everything here.
	// if (request.httpVersion != "HTTP/1.0" || request.httpVersion != "HTTP/1.1"  || !::is_methodvalid(request.method))
	// {
	// 	this->set_status(BadRequest);
	// 	return ;
	// }
	// this->resolve(); // Gets the interpreter path, gets the path to the script, query_string, identifies if it is cgi or a normal request..
	// Suppose u have: interpreter_path, script_path, query_string (p1=0&p2=1...), iscgi
	// NOTE: this is only to prevent double call bc am hardcoding it for now!
	if (request.uri != "/images/main.py?param1=hello&param2=world")
		return;
	/*
		getting the extantion -> check the config for the matching route if has the cgi block -> 
										-> yes?: this is a cgi
										-> no?: reads it as a raw text ? ofc it not exist 404.
	*/
	// TODO: check if it is cgi.
	std::string e = ".py";
	std::string url = "main.py";
	std::map<std::string, std::string> params;
	params.insert(std::make_pair("param1", "hello"));
    params.insert(std::make_pair("param2", "world"));
	if(url.length() > e.length() && &url[url.length() - e.length()] == e)
	{
		std::cout << "------- CGI -------" << std::endl;
		std::cout << "extension: " << e  << std::endl;
		std::cout << "uri: " << request.uri  << std::endl;
		std::cout << "param1: " << params.at("param1") << "\nparam2: " << params.at("param2")  << std::endl;
		std::cout << "-------------------" << std::endl;
	}
	this->stage = SendingCgi;
}

Response::~Response()
{
}

void Response::serialize_headers(void)
{
	this->headers_as_str += this->status_line;
	for (std::map<std::string, std::string>::iterator it = this->headers.begin(); it != this->headers.end(); ++it)
		this->headers_as_str += it->first + ": " + it->second;
	this->headers_as_str += "\r\n";
	this->bytes_sent = 0;
}


void Response::send_headers(void) __THROWS_STRERROR
{
}


//
// // status line // HTTP/1.0 200 OK
//
bool Response::isdone(void)
{
	// TODO: What if the body was not sent yet??
	// what if it is a file? cgi?..
	return (true);
}
//
// void Response::write(int conn) __THROWS_STRERROR
// {
// 	ssize_t count;
// 	size_t  write_size;
//
// 	if (!this->__is_serialized)
// 		this->serialize(); // NOTE: converts headers and body into client writable form in __serialized_response
//
// 	write_size = WRITE_CHUNK_SIZE;
// 	if (write_size > this->__serialized_response.size() - this->sent)
// 		write_size = this->__serialized_response.size() - this->sent;
//
// 	count = ::write(conn,
// 		this->__serialized_response.c_str() + this->sent,
// 		write_size);
// 	if (count <= 0) throw strerror(errno);
// 	this->sent += count;
// 	// TODO: Well, lazy loading files is probably better.
// 	// html files, audio, video files. should be loaded.
// }

void Response::init_status_lines()
{
    Response::status_lines[OK                 ] = "HTTP/1.0 200 OK";
    Response::status_lines[Created            ] = "HTTP/1.0 201 Created";
    Response::status_lines[NoContent          ] = "HTTP/1.0 204 No Content";
    Response::status_lines[MovedPermanently   ] = "HTTP/1.0 301 Moved Permanently";
    Response::status_lines[Found              ] = "HTTP/1.0 302 Found";
    Response::status_lines[NotModified        ] = "HTTP/1.0 304 Not Modified";
    Response::status_lines[BadRequest         ] = "HTTP/1.0 400 Bad Request";
    Response::status_lines[Unauthorized       ] = "HTTP/1.0 401 Unauthorized";
    Response::status_lines[Forbidden          ] = "HTTP/1.0 403 Forbidden";
    Response::status_lines[NotFound           ] = "HTTP/1.0 404 Not Found";
    Response::status_lines[MethodNotAllowed   ] = "HTTP/1.0 405 Method Not Allowed";
    Response::status_lines[RequestTimeout     ] = "HTTP/1.0 408 Request Timeout";
    Response::status_lines[InternalServerError] = "HTTP/1.0 500 Internal Server Error";
	// Cgi?
    Response::status_lines[NotImplemented     ] = "HTTP/1.0 501 Not Implemented";
    Response::status_lines[BadGateway         ] = "HTTP/1.0 502 Bad Gateway";
    Response::status_lines[ServiceUnavailable ] = "HTTP/1.0 503 Service Unavailable";
}

void Response::appendheader(const std::string key, const std::string value)
{
	this->headers[key] = value + "\r\n";
}

void Response::set_status(int s)
{
	this->status = s;
	this->status_line =
		Response::status_lines[this->status] + "\r\n";
	// TODO: fetch error page?
}

int Response::get_status(void) const
{
	return (this->status);
}

response_stage_t Response::getstage(void) const
{
	return (this->stage);
}
