#include "HTTP/Response.hpp"
#include <Server.hpp>
#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <utility>

std::map<int, std::string> Response::status_lines;
std::map<std::string, std::string> Response::mimes;

static std::string trim_uri_to_path(const std::string &uri)
{
	if (uri.empty()) return "/";
	size_t end = uri.find_first_of("?#");
	std::string path = uri.substr(0, end);
	if (path.empty()) return "/";
	if (path[0] != '/') path = "/" + path;

	std::string normalized;
	normalized.reserve(path.size());
	bool last_was_slash = false;
	for (size_t index = 0; index < path.size(); ++index)
	{
		if (path[index] == '/')
		{
			if (last_was_slash) continue;
			last_was_slash = true;
		}
		else
			last_was_slash = false;
		normalized.push_back(path[index]);
	}

	if (normalized.empty()) return "/";
	return normalized;
}

static bool location_matches(const std::string &request_path, const std::string &location_uri)
{
	if (location_uri.empty()) return false;
	if (location_uri == "/") return true;
	if (request_path.compare(0, location_uri.size(), location_uri) != 0) return false;
	if (request_path.size() == location_uri.size()) return true;
	if (location_uri[location_uri.size() - 1] == '/') return true;
	return request_path[location_uri.size()] == '/';
}

static std::string join_fs_path(const std::string &root, const std::string &suffix)
{
	std::string base = root.empty() ? "." : root;
	if (suffix.empty()) return base;

	if (base[base.size() - 1] == '/' && suffix[0] == '/')
		return base + suffix.substr(1);
	if (base[base.size() - 1] != '/' && suffix[0] != '/')
		return base + "/" + suffix;
	return base + suffix;
}

static std::string reason_phrase_for_status(int code)
{
	std::map<int, std::string>::iterator it = Response::status_lines.find(code);
	if (it == Response::status_lines.end())
	{
		std::stringstream c;
		c << code;
		std::string auto_reason = "HTTP/1.0 " + c.str() + " Error";
		return auto_reason;
	}
	return it->second;
}

static std::string build_default_error_html(int code)
{
    std::string reason = reason_phrase_for_status(code);
    std::stringstream ss;
    
    ss << "<!DOCTYPE html>\n"
       << "<html lang=\"en\">\n"
       << "<head>\n"
       << "    <meta charset=\"UTF-8\">\n"
       << "    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n"
       << "    <title>" << code << " " << reason << "</title>\n"
       << "    <style>\n"
       << "        body {\n"
       << "            font-family: system-ui, -apple-system, \"Segoe UI\", Roboto, sans-serif;\n"
       << "            background-color: #f3f4f6;\n"
       << "            color: #1f2937;\n"
       << "            display: flex;\n"
       << "            justify-content: center;\n"
       << "            align-items: center;\n"
       << "            height: 100vh;\n"
       << "            margin: 0;\n"
       << "        }\n"
       << "        .error-container {\n"
       << "            background-color: #ffffff;\n"
       << "            padding: 3rem 4rem;\n"
       << "            border-radius: 8px;\n"
       << "            box-shadow: 0 4px 6px -1px rgba(0, 0, 0, 0.1), 0 2px 4px -1px rgba(0, 0, 0, 0.06);\n"
       << "            text-align: center;\n"
       << "            max-width: 500px;\n"
       << "            width: 90%;\n"
       << "        }\n"
       << "        h1 {\n"
       << "            font-size: 5rem;\n"
       << "            margin: 0;\n"
       << "            color: #ef4444;\n"
       << "            line-height: 1;\n"
       << "        }\n"
       << "        h2 {\n"
       << "            font-size: 1.5rem;\n"
       << "            margin-top: 1rem;\n"
       << "            font-weight: 500;\n"
       << "            color: #4b5563;\n"
       << "        }\n"
       << "        hr {\n"
       << "            border: none;\n"
       << "            border-top: 1px solid #e5e7eb;\n"
       << "            margin: 2rem 0;\n"
       << "        }\n"
       << "        .server-footer {\n"
       << "            color: #9ca3af;\n"
       << "            font-size: 0.875rem;\n"
       << "            margin: 0;\n"
       << "        }\n"
       << "    </style>\n"
       << "</head>\n"
       << "<body>\n"
       << "    <div class=\"error-container\">\n"
       << "        <h1>" << code << "</h1>\n"
       << "        <h2>" << reason << "</h2>\n"
       << "        <hr>\n"
       << "        <p class=\"server-footer\">webserv</p>\n"
       << "    </div>\n"
       << "</body>\n"
       << "</html>\n";

    return ss.str();
}

static bool is_methodvalid(const std::string &method)
{
	return ((method == "GET") || (method == "POST") || (method == "DELETE"));
}

void Response::init_mimes()
{
	if (!mimes.empty()) return ;
	Response::mimes["html"]  =  TextHtml          ;
	Response::mimes["txt"]   =  TextPlain         ;
	Response::mimes["css"]   =  TextCss           ;
	Response::mimes["js"]    =  TextJavascript    ;  // what
	Response::mimes["xml"]   =  TextXml           ;  // what v2
	Response::mimes["csv"]   =  TextCsv           ;
	Response::mimes["jpg"]   =  ImageJpeg         ;
	Response::mimes["png"]   =  ImagePng          ;
	Response::mimes["gif"]   =  ImageGif          ;
	Response::mimes["webp"]  =  ImageWebp         ;
	Response::mimes["svg"]   =  ImageSvg          ;
	Response::mimes["ico"]   =  ImageIco          ;
	Response::mimes["json"]  =  ApplicationJson   ;
	Response::mimes["xml"]   =  ApplicationXml    ;  // what v2
	Response::mimes["pdf"]   =  ApplicationPdf    ;
	Response::mimes["zip"]   =  ApplicationZip    ;
	Response::mimes["bin"]   =  ApplicationOctet  ;
	Response::mimes["form"]  =  ApplicationForm   ;
	Response::mimes["js"]    =  ApplicationJs     ;  // what
	Response::mimes["mp3"]   =  AudioMpeg         ;  // what v3
	Response::mimes["ogg"]   =  AudioOgg          ;
	Response::mimes["mp3"]   =  AudioMp3          ;  // what v3
	Response::mimes["mp4"]   =  VideoMp4          ;
	Response::mimes["webm"]  =  VideoWebm         ;
}

Response::Response()
{
	this->stage = Setup;
	this->resolved_path = "";
}

void Response::continue_processing(const HttpRequest &request)
{
	if (this->stage == Setup)
	{
		this->setup_response(request);
		this->serialize_headers();
		::write(request.conn, this->headers_as_str.c_str(), this->headers_as_str.size());
		this->stage = SendingResource;
	}
	if (this->stage == SendingResource) {
		this->resource.send(request);
		return ;
	}
	abort();
}

void Response::setup_response(const HttpRequest &request)
{
	if ((request.httpVersion != "HTTP/1.0" && request.httpVersion != "HTTP/1.1") || !is_methodvalid(request.method))
	{
		this->set_status(BadRequest);
		this->get_error_page_html(request, BadRequest); // TODO: need to make appropriate headers ig
		return ;
	}
	this->set_status(OK);
	this->appendheader("content-type", TextHtml);
	std::cout << "|" << request.httpVersion << "|\n";
	std::cout << "Condition: " << (request.httpVersion != "HTTP/1.0" && request.httpVersion != "HTTP/1.1") << "\n";
	std::cout << "Condition: " << !is_methodvalid(request.method) << "\n";
	std::cout << "Method: " << request.method << "\n";

	UriResolutionResult resolved = Response::resolve_uri_to_path(request);
	// We should check the method??

	// std::cout << "GOT: " << resolved.filesystem_path << "\n";
	// std::cout << "ROOT: " << resolved.root << "\n";
	this->resolved_path = resolved.filesystem_path;
	// TODO: check if it is cgi.
	std::string e = ".py";
	if(request.uri.length() > e.length() && &request.uri[request.uri.length() - e.length()] == e)
	{
		std::cout << "------- CGI -------" << std::endl;
		std::cout << "extension: " << e << std::endl;
		std::cout << "uri: " << request.uri  << std::endl;
		std::map<std::string, std::string>::iterator it;
		std::map<std::string, std::string> params = request.params;
		for (it = params.begin(); it != params.end(); ++it) {
			std::cout << "Key: " << it->first << " |  Value: " << it->second << std::endl;
		}
		std::cout << "-------------------" << std::endl;
	}
	// this->resource.setresource_type(Text);
	// this->resource.setresource_type(Cgi);
	// this->resource.setresource_type(File);
}

const std::string Response::get_error_page_html(const HttpRequest &request, int code)
{
	const ServerConfig &server_conf = Multiplexer::confs[request.owner];
	std::map<size_t, std::string>::const_iterator configured = server_conf.error_pages.find(static_cast<size_t>(code));
	if (configured != server_conf.error_pages.end())
	{
		std::string configured_path = join_fs_path(server_conf.root, configured->second);
		std::ifstream stream(configured_path.c_str());
		if (stream.is_open())
		{
			std::ostringstream content;
			content << stream.rdbuf();
			this->resource.set_stream_buffer(content.str());
			this->resource.setresource_type(Text);
			this->resource.identify_type(configured_path);
			return this->resource.get_stream_buffer();
		}
	}

	this->resource.set_stream_buffer(build_default_error_html(code));
	this->resource.setresource_type(Text);
	this->resource.identify_type("error.html");
	return this->resource.get_stream_buffer();
	return resource.get_stream_buffer();
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

void Response::appendheader(const char *key, const char  *value)
{
	this->headers[key] = value;
	this->headers[key] += "\r\n";
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

const std::string &Response::get_resolved_resource_path(void) const
{
	return this->resolved_path;
}

UriResolutionResult Response::resolve_uri_to_path(const HttpRequest &request)
{
	UriResolutionResult resolved;
	resolved.matched_location = "";
	resolved.request_path = trim_uri_to_path(request.uri);
	resolved.filesystem_path = resolved.request_path;

	const ServerConfig &server_conf = Multiplexer::confs[request.owner];
	resolved.root = server_conf.root;
	resolved.index = server_conf.index;

	const LocationConfig *best_location = NULL;
	for (size_t i = 0; i < server_conf.locations.size(); ++i)
	{
		const LocationConfig &current = server_conf.locations[i];
		if (!location_matches(resolved.request_path, current.uri)) continue;
		if (!best_location || current.uri.size() > best_location->uri.size())
			best_location = &current;
	}

	std::string relative_uri = resolved.request_path;
	if (best_location)
	{
		resolved.matched_location = best_location->uri;
		if (!best_location->root.empty()) resolved.root = best_location->root;
		if (!best_location->index.empty()) resolved.index = best_location->index;

		if (best_location->uri != "/")
		{
			if (resolved.request_path.size() <= best_location->uri.size())
				relative_uri = "/";
			else
				relative_uri = resolved.request_path.substr(best_location->uri.size());
		}
	}

	if (relative_uri.empty() || relative_uri[0] != '/')
		relative_uri = "/" + relative_uri;

	resolved.filesystem_path = join_fs_path(resolved.root, relative_uri);
	if (!resolved.index.empty())
	{
		bool needs_index = false;
		if (relative_uri == "/") needs_index = true;
		else if (!resolved.request_path.empty() && resolved.request_path[resolved.request_path.size() - 1] == '/')
			needs_index = true;
		if (needs_index)
			resolved.filesystem_path = join_fs_path(resolved.filesystem_path, resolved.index);
	}

	// resolved path -> 
	// Check if the path leads to:
	// Cgi..
	//     cgi_pass 
	// Directory..
	// Static File

	return resolved;
}

response_stage_t Response::getstage(void) const
{
	return (this->stage);
}
