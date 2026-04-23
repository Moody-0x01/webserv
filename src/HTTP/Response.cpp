 #include "HTTP/Response.hpp"
#include <Server.hpp>
#include <dirent.h>
#include <cstdlib>
#include <cerrno>
#include <fstream>
#include <ios>
#include <iostream>
#include <map>
#include <ostream>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <utility>
#include <vector>

std::map<int, std::string> Response::status_lines;
std::map<std::string, std::string> Response::mimes;

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

// if no index and auto index, just match the cgi
static std::string extract_extension(const std::string &path)
{
	size_t slash = path.find_last_of('/');
	size_t dot = path.find_last_of('.');
	if (dot == std::string::npos || (slash != std::string::npos && dot < slash))
		return std::string();
	return path.substr(dot);
}

static UriResolutionResult::type get_resource_type(const std::string &path)
{
	struct stat path_stat;

	if (::stat(path.c_str(), &path_stat) != 0) return UriResolutionResult::None;
	if (S_ISDIR(path_stat.st_mode))
		return UriResolutionResult::directory;
	else if (S_ISREG(path_stat.st_mode))
		return UriResolutionResult::file;
	return UriResolutionResult::None;
}

static const LocationConfig *find_best_location(const ServerConfig &server_conf, const std::string &request_path)
{
	const LocationConfig *best_location = NULL;
	for (size_t i = 0; i < server_conf.locations.size(); ++i)
	{
		const LocationConfig &current = server_conf.locations[i];
		if (!location_matches(request_path, current.uri)) continue;
		if (!best_location || current.uri.size() > best_location->uri.size())
			best_location = &current;
	}
	return best_location;
}

static std::string compute_relative_uri(const std::string &request_path, const LocationConfig *best_location)
{
	std::string relative_uri = request_path;
	if (best_location && best_location->uri != "/" && !best_location->root.empty())
	{
		if (request_path.size() <= best_location->uri.size())
			relative_uri = "/";
		else
			relative_uri = request_path.substr(best_location->uri.size());
	}

	if (relative_uri.empty() || relative_uri[0] != '/')
		relative_uri = "/" + relative_uri;
	return relative_uri;
}

static void apply_location_override(UriResolutionResult &resolved, const LocationConfig *best_location)
{
	if (!best_location) return;
	resolved.matched_location = best_location;
	if (!best_location->root.empty()) resolved.root = best_location->root;
	if (!best_location->index.empty()) resolved.index = best_location->index;
}

static std::string build_filesystem_target(const UriResolutionResult &resolved, const std::string &relative_uri)
{
	std::string filesystem_path = join_fs_path(resolved.root, relative_uri);
	if (!resolved.index.empty())
	{
		bool needs_index = false;
		if (relative_uri == "/") needs_index = true;
		else if (!resolved.request_path.empty() && resolved.request_path[resolved.request_path.size() - 1] == '/')
			needs_index = true;
		if (needs_index)
			filesystem_path = join_fs_path(filesystem_path, resolved.index);
	}
	return filesystem_path;
}

static void resolve_cgi_script(UriResolutionResult &resolved, const LocationConfig *best_location)
{
	if (!best_location || best_location->cgi_path.empty()) return;
	std::string extension = extract_extension(resolved.filesystem_path);

	std::map<std::string, std::string>::const_iterator cgi_it = best_location->cgi_path.find(extension);
	if (!extension.empty() && cgi_it != best_location->cgi_path.end())
	{
		resolved.resource_type = UriResolutionResult::cgi;
		resolved.cgi_script = std::make_pair(resolved.filesystem_path, cgi_it->second);
		if (!exists(resolved.filesystem_path)) {
			resolved.resource_type = UriResolutionResult::None;
		}
	}
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

static std::string escape_html(const std::string &input)
{
	std::string escaped;
	escaped.reserve(input.size());
	for (size_t i = 0; i < input.size(); ++i)
	{
		const char c = input[i];
		switch (c)
		{
			case '&': escaped += "&amp;"; break;
			case '<': escaped += "&lt;"; break;
			case '>': escaped += "&gt;"; break;
			case '"': escaped += "&quot;"; break;
			case '\'': escaped += "&#39;"; break;
			default: escaped += c; break;
		}
	}
	return escaped;
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

Response::MethodKind Response::classify_method(const std::string &method)
{
	if (method == "GET") return MethodGet;
	if (method == "POST") return MethodPost;
	if (method == "DELETE") return MethodDelete;
	return MethodInvalid;
}

void Response::init_mimes() {
	if (!mimes.empty())
		return;
	Response::mimes["md"] = TextMarkDown;
	Response::mimes["html"] = TextHtml;
	Response::mimes["txt"] = TextPlain;
	Response::mimes["css"] = TextCss;
	Response::mimes["csv"] = TextCsv;
	Response::mimes["jpg"] = ImageJpeg;
	Response::mimes["png"] = ImagePng;
	Response::mimes["gif"] = ImageGif;
	Response::mimes["webp"] = ImageWebp;
	Response::mimes["svg"] = ImageSvg;
	Response::mimes["ico"] = ImageIco;
	Response::mimes["json"] = ApplicationJson;
	Response::mimes["xml"] = ApplicationXml;
	Response::mimes["pdf"] = ApplicationPdf;
	Response::mimes["zip"] = ApplicationZip;
	Response::mimes["bin"] = ApplicationOctet;
	Response::mimes["js"] = ApplicationJs;
	Response::mimes["mp3"] = Audio;
	Response::mimes["mp4"] = VideoMp4;
	Response::mimes["webm"] = VideoWebm;
}

Response::Response()
{
	this->stage = Setup;
	this->request_ptr = NULL;
	this->status = OK;
	this->status_line = "HTTP/1.0 200 OK\r\n";
	this->bytes_sent = 0;
}

UriResolutionResult::UriResolutionResult()
{
	resource_type = file;
	matched_location = NULL;
	request_path = "/";
	root = "";
	index = "";
	filesystem_path = "/";
	cgi_script = std::make_pair(std::string(), std::string());
}

Resource &Response::get_resource_ref(void) { return (this->resource);};

/*  []  */
/*  [headers | body]  */

void Response::continue_processing(const HttpRequest &request) __THROWS_STRERROR
{
	if (!this->request_ptr)
		this->request_ptr = &request;
	if (this->stage == Setup)
	{
		this->setup_response(request);
		if (this->resource.getresource_type() == CGI) {
			try {
				this->resource.cgi.execute();
				this->stage = ProcessingCgi;
			} catch (const char *e) {
				this->set_status(InternalServerError);
				throw e;
			}
		}
	}
	if (this->stage == SendingResource)
	{
		this->send_headers(request.conn);
		this->stage = this->resource.send(request);
	} else if (this->stage == ProcessingCgi) {	
		if (this->resource.cgi.timeout())
		{
			if (!this->resource.cgi.headers_sent) {
				std::cout << "Timout but headers were not sent.\n";
				this->set_status(RequestTimeout);
				return ;
			}
			std::cout << "Timout but headers already sent.\n";
			this->stage = DoneSending;
			return ;
		}
		if (!this->resource.cgi.headers_sent && this->resource.cgi.headers_parsed) {
			this->resource.cgi
				.send_headers(request.conn);
		} else if (this->resource.cgi.headers_sent && this->resource.cgi.state == ReadingBody) {
			this->resource.cgi
				.send_body_chunk(request.conn);
		} else if (this->resource.cgi.state == DONE && !this->resource.cgi.headers_sent) {
			if (this->resource.cgi.did_fail()) {
				this->set_status(BadGateway);
			} else
				this->set_status(InternalServerError);
			return ;
		}
		if (this->resource.cgi.state == DONE)
			this->stage = DoneSending;
	}
}

bool Response::is_method_allowed(std::string method)
{
	if (!resolved_results.matched_location || resolved_results.matched_location->methods.empty())
	{
		if (method == "GET" || method == "POST" || method == "DELETE") return (true);
		else return false;
	}
	for (size_t i = 0; i < resolved_results.matched_location->methods.size(); ++i)
	{
		if (resolved_results.matched_location->methods[i] == method)
			return (true);
	}
	return (false);
}

void Response::setup_response(const HttpRequest &request)
{
	if ((request.httpVersion != "HTTP/1.0" && request.httpVersion != "HTTP/1.1"))
	{
		this->set_status(BadRequest);
		return ;
	}
	if (request.isbadrequest)
	{
		this->set_status(request.code);
		return ;
	}	
	this->resolved_results = Response::resolve_uri_to_path(request);
	if (!this->is_method_allowed(request.method))
	{
		this->set_status(MethodNotAllowed);
		return ;
	}
	if (this->resolved_results.resource_type == UriResolutionResult::None)
	{
		this->set_status(NotFound);
		return ;
	}

	if (this->resolved_results.resource_type == UriResolutionResult::cgi)
	{
		this->resource.setresource_type(CGI);
		this->resource.cgi.setup(request, 
			this->resolved_results.cgi_script.first, 
			this->resolved_results.cgi_script.second);
		return ;
	}
	switch (Response::classify_method(request.method))
	{
		// Note: any method other than Get in this section is MethodNotAllowed
		case MethodGet:
			this->handle_get(request);
			break ;
		case MethodPost:
			this->handle_post(request);
			break ;
		case MethodDelete:
			this->handle_delete(request);
			break ;
		default:
			this->set_status(BadRequest);
			break;
	}
}

void Response::list_dir(void)
{
	const std::string &directory_path = this->resolved_results.filesystem_path;
	DIR *directory = ::opendir(directory_path.c_str());
	if (!directory)
	{
		if (errno == EACCES)
			this->set_status(Forbidden);
		else
			this->set_status(InternalServerError);
		return;
	}

	std::vector< std::pair<std::string, bool> > entries;
	struct dirent *entry;
	while ((entry = ::readdir(directory)) != NULL)
	{
		std::string name = entry->d_name;
		if (name == ".") continue;
		bool is_directory = false;
		std::string full_entry_path = join_fs_path(directory_path, name);
		struct stat entry_stat;
		if (::stat(full_entry_path.c_str(), &entry_stat) == 0)
			is_directory = S_ISDIR(entry_stat.st_mode);
		entries.push_back(std::make_pair(name, is_directory));
	}
	::closedir(directory);
	std::sort(entries.begin(), entries.end());

	std::string request_uri = this->resolved_results.request_path;
	if (request_uri.empty()) request_uri = "/";
	if (request_uri[request_uri.size() - 1] != '/') request_uri += "/";

	std::ostringstream body;
	body << "<!DOCTYPE html>\n"
		 << "<html lang=\"en\">\n"
		 << "<head><meta charset=\"UTF-8\"><title>Index of " << escape_html(request_uri) << "</title></head>\n"
		 << "<body>\n"
		 << "<h1>Index of " << escape_html(request_uri) << "</h1>\n"
		 << "<hr>\n"
		 << "<ul>\n";

	for (size_t i = 0; i < entries.size(); ++i)
	{
		const std::string &name = entries[i].first;
		const bool is_directory = entries[i].second;
		std::string display_name = name;
		std::string href = request_uri + name;
		if (is_directory)
		{
			display_name += "/";
			href += "/";
		}
		body << "<li><a href=\"" << escape_html(href) << "\">"
			 << escape_html(display_name) << "</a></li>\n";
	}

	body << "</ul>\n"
		 << "<hr>\n"
		 << "</body>\n"
		 << "</html>\n";

	this->resource.setresource_type(Dir);
	this->resource.identify_type("index.html");
	this->resource.set_stream_buffer(body.str());
}

void Response::serve_file(void)
{
	const std::string &file_path = this->resolved_results.filesystem_path;
	int open_status = this->resource.open(file_path);

	if (open_status != OK)
	{
		if (open_status == Unauthorized)
			this->set_status(Forbidden);
		else
			this->set_status(open_status);
	}
	else {
		this->set_status(OK);
	}

	std::string content_type = this->resource.getmime_type();
	this->appendheader("Content-Type", content_type.c_str());
}

void Response::handle_get(const HttpRequest &request)
{
	const ServerConfig &server_conf = Multiplexer::get_conf(request.owner);

	if (this->resolved_results.resource_type == UriResolutionResult::directory)
	{
		if ((resolved_results.matched_location && resolved_results.matched_location->autoindex) || (!resolved_results.matched_location && server_conf.autoindex))
			list_dir();
		else
			this->set_status(NotFound);
	}
	else
		serve_file();
	this->stage = SendingResource;
}

void Response::handle_post(const HttpRequest &request)
{
	if (request.content_length == 0)
	{
		this->set_status(ContentLengthRequired);
		return;
	}

	std::string file = this->resolved_results.filesystem_path;
	if (this->resolved_results.resource_type == UriResolutionResult::directory)
	{
		this->set_status(Forbidden);
		return;
	}

	std::fstream out_file(file.c_str(), std::ios::out | std::ios::binary | std::ios::trunc);
	if (!out_file.is_open())
	{
		this->set_status(InternalServerError);
		return;
	}

	out_file.write(request.body.c_str(), request.body.size());
	if (out_file.fail())
	{
		out_file.close();
		this->set_status(InternalServerError);
		return;
	}
	this->set_status(Created);
	this->resource.setresource_type(Text);
	this->resource.setmime_type(TextHtml);
	this->resource.set_stream_buffer(
        "<!DOCTYPE html>\n"
        "<html>\n"
        "<head><title>201 Created</title></head>\n"
        "<body><h1>201 Created</h1><p>File successfully uploaded.</p></body>\n"
        "</html>\n"
    );
}

void Response::handle_delete(const HttpRequest &request)
{
	(void)request;
}

void Response::get_error_page_html(const HttpRequest &request, int code)
{
	const ServerConfig &server_conf = Multiplexer::get_conf(request.owner);
	std::map<size_t, std::string>::const_iterator configured = server_conf.error_pages.find(static_cast<size_t>(code));


	this->resource.setresource_type(Text);
	this->resource.setmime_type(TextHtml);
	if (configured != server_conf.error_pages.end())
	{
		std::string configured_path = join_fs_path(server_conf.root, configured->second);
		std::ifstream stream(configured_path.c_str());
		if (stream.is_open())
		{
			std::ostringstream content;
			content << stream.rdbuf();
			this->resource
				.set_stream_buffer(content.str());
			return ;
		}
	}
	this->resource
		.set_stream_buffer(build_default_error_html(code));
}

Response::~Response()
{
}

void Response::serialize_headers(void) {

	this->headers_as_str = (this->status_line + ::serialize_headers(this->headers, false));
	this->bytes_sent = 0;
}


void Response::send_headers(int conn) __THROWS_STRERROR
{
	this->serialize_headers();
	Multiplexer::write(conn, this->headers_as_str.c_str(), this->headers_as_str.size());
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

	Response::status_lines[ContentLengthRequired] = "HTTP 411 Length Required";
    Response::status_lines[NotImplemented       ] = "HTTP/1.0 501 Not Implemented";
    Response::status_lines[BadGateway           ] = "HTTP/1.0 502 Bad Gateway";
    Response::status_lines[ServiceUnavailable   ] = "HTTP/1.0 503 Service Unavailable";
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
	if (s != OK) {
		this->get_error_page_html(*this->request_ptr, s);
		this->appendheader("Content-Type", this->resource.getmime_type().c_str());
		this->stage = SendingResource;
	}
}

int Response::get_status(void) const
{
	return (this->status);
}

const UriResolutionResult &Response::get_resolved_results(void) const
{
	return this->resolved_results;
}

UriResolutionResult Response::resolve_uri_to_path(const HttpRequest &request)
{
	UriResolutionResult resolved;
	resolved.request_path = request.uri.empty() ? "/" : request.uri;
	resolved.filesystem_path = resolved.request_path;

	const ServerConfig &server_conf = Multiplexer::get_conf(request.owner);
	resolved.root = server_conf.root;
	resolved.index = server_conf.index;

	const LocationConfig *best_location = find_best_location(server_conf, resolved.request_path);
	apply_location_override(resolved, best_location);

	std::string relative_uri = compute_relative_uri(resolved.request_path, best_location);
	resolved.filesystem_path = build_filesystem_target(resolved, relative_uri);
	resolved.resource_type = get_resource_type(resolved.filesystem_path);
	if (resolved.resource_type == UriResolutionResult::file)
		resolve_cgi_script(resolved, best_location);
	return resolved;
}

response_stage_t Response::getstage(void) const
{
	return (this->stage);
}
