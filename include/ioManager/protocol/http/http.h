//referenced from: https://github.com/nodejs/llhttp
//
//2025.03.13
#pragma once
#include "../../ioManager.h"

namespace io
{
    inline namespace IO_LIB_VERSION___
    {
        namespace prot
        {
            namespace http
            {
                namespace llhttp{
#include "llhttp/llhttp.h"
#ifdef _MSC_VER
#pragma warning(disable: 4065)
#endif
#include "llhttp/llhttp.c"
#ifdef _MSC_VER
#pragma warning(default: 4065)
#endif
#include "llhttp/http.c"
inline int case_insensitive_compare(const char* s1, const char* s2) {
#ifdef WIN32
                    return _stricmp(s1, s2);
#else
                    return strcasecmp(s1, s2);
#endif
                    }
                };
                
                // HTTP request in-situ structure (holds references to original buffers)
                struct req_insitu {
                    llhttp::llhttp_method_t method;
                    std::string_view url;
                    std::uint8_t major_version;
                    std::uint8_t minor_version;
                    
                    std::map<std::string_view, std::string_view> headers;
                    
                    std::vector<std::string_view> body_fragments;
                    
                    std::vector<io::buf> raw_buffers;
                    
                    // Get header value by name (case-insensitive)
                    inline std::string_view get_header(const std::string_view& name) const {
                        for (const auto& [key, value] : headers) {
                            if (llhttp::case_insensitive_compare(key.data(), name.data()) == 0) {
                                return value;
                            }
                        }
                        return {};
                    }
                    
                    // Convert method to string
                    inline std::string_view method_name() const {
                        return llhttp::llhttp_method_name(method);
                    }
                    
                    // Get body size (combined size of all fragments)
                    inline size_t body_size() const {
                        size_t total_size = 0;
                        for (const auto& fragment : body_fragments) {
                            total_size += fragment.size();
                        }
                        return total_size;
                    }

                    req_insitu() = default;
                    req_insitu(req_insitu&&) = default;
                    req_insitu& operator=(req_insitu&&) = default;

                    IO_MANAGER_BAN_COPY(req_insitu);
                };

                // HTTP response in-situ structure (holds references to original buffers)
                struct rsp_insitu {
                    int status_code;
                    std::string_view status_message;
                    std::uint8_t major_version;
                    std::uint8_t minor_version;
                    
                    std::map<std::string_view, std::string_view> headers;
                    
                    std::vector<std::string_view> body_fragments;
                    
                    std::vector<io::buf> raw_buffers;

                    // Get header value by name (case-insensitive)
                    inline std::string_view get_header(const std::string_view& name) const {
                        for (const auto& [key, value] : headers) {
                            if (llhttp::case_insensitive_compare(key.data(), name.data()) == 0) {
                                return value;
                            }
                        }
                        return {};
                    }
                    
                    // Get body size (combined size of all fragments)
                    inline size_t body_size() const {
                        size_t total_size = 0;
                        for (const auto& fragment : body_fragments) {
                            total_size += fragment.size();
                        }
                        return total_size;
                    }

                    rsp_insitu() = default;
                    rsp_insitu(rsp_insitu&&) = default;
                    rsp_insitu& operator=(rsp_insitu&&) = default;

                    IO_MANAGER_BAN_COPY(rsp_insitu);
                };
                
                // HTTP request string structure
                struct req
                {
                    std::string method;
                    std::string url;
                    uint8_t major_version;
                    uint8_t minor_version;
                    
                    std::map<std::string, std::string> headers;
                    
                    std::string body;
                    
                    // Default constructor
                    inline req() = default;
                    
                    // Constructor from in-situ request
                    inline req(const req_insitu& insitu) 
                        : method(std::string(insitu.method_name()))
                        , url(insitu.url)
                        , major_version(insitu.major_version)
                        , minor_version(insitu.minor_version)
                    {
                        // Copy headers
                        for (const auto& [key, value] : insitu.headers) {
                            headers[std::string(key)] = std::string(value);
                        }
                        
                        // Combine body fragments into a single string
                        if (!insitu.body_fragments.empty()) {
                            size_t total_size = insitu.body_size();
                            body.reserve(total_size);
                            
                            for (const auto& fragment : insitu.body_fragments) {
                                body.append(fragment);
                            }
                        }
                    }

                    // Serialize a string-based request to a single buffer
                    io::buf serialize() {
                        auto& request = *this;
                        // Calculate total size needed
                        size_t total_size = 0;

                        // Request line
                        total_size += request.method.size() + 1; // method + space
                        total_size += request.url.size() + 1;    // url + space
                        total_size += 5;                         // "HTTP/"
                        total_size += 1;                         // major version
                        total_size += 1;                         // "."
                        total_size += 1;                         // minor version
                        total_size += 2;                         // "\r\n"

                        // Headers
                        for (const auto& [key, value] : request.headers) {
                            total_size += key.size() + 2;        // key + ": "
                            total_size += value.size() + 2;      // value + "\r\n"
                        }

                        // Check if Content-Length header is needed
                        bool has_content_length = false;
                        if (!request.body.empty()) {
                            for (const auto& [key, value] : request.headers) {
                                if (llhttp::case_insensitive_compare(key.c_str(), "Content-Length") == 0) {
                                    has_content_length = true;
                                    break;
                                }
                            }

                            if (!has_content_length) {
                                // Content-Length: + number + \r\n
                                std::string content_length = std::to_string(request.body.size());
                                total_size += 16 + content_length.size() + 2; // "Content-Length: " + size + "\r\n"
                            }
                        }

                        // End of headers line
                        total_size += 2;                         // "\r\n"

                        // Body
                        total_size += request.body.size();

                        // Allocate buffer of the calculated size
                        io::buf buffer(total_size);
                        char* ptr = static_cast<char*>(buffer.data());
                        size_t remaining = total_size;

                        // Copy request line
                        size_t len = request.method.size();
                        std::memcpy(ptr, request.method.data(), len);
                        ptr += len;
                        remaining -= len;

                        *ptr++ = ' ';
                        remaining--;

                        len = request.url.size();
                        std::memcpy(ptr, request.url.data(), len);
                        ptr += len;
                        remaining -= len;

                        *ptr++ = ' ';
                        remaining--;

                        const char* http_ver = "HTTP/";
                        len = 5; // Length of "HTTP/"
                        std::memcpy(ptr, http_ver, len);
                        ptr += len;
                        remaining -= len;

                        *ptr++ = '0' + request.major_version;
                        remaining--;

                        *ptr++ = '.';
                        remaining--;

                        *ptr++ = '0' + request.minor_version;
                        remaining--;

                        *ptr++ = '\r';
                        *ptr++ = '\n';
                        remaining -= 2;

                        // Copy headers
                        for (const auto& [key, value] : request.headers) {
                            len = key.size();
                            std::memcpy(ptr, key.data(), len);
                            ptr += len;
                            remaining -= len;

                            *ptr++ = ':';
                            *ptr++ = ' ';
                            remaining -= 2;

                            len = value.size();
                            std::memcpy(ptr, value.data(), len);
                            ptr += len;
                            remaining -= len;

                            *ptr++ = '\r';
                            *ptr++ = '\n';
                            remaining -= 2;
                        }

                        // Add Content-Length header if needed
                        if (!request.body.empty() && !has_content_length) {
                            const char* cl_header = "Content-Length: ";
                            len = 16; // Length of "Content-Length: "
                            std::memcpy(ptr, cl_header, len);
                            ptr += len;
                            remaining -= len;

                            std::string content_length = std::to_string(request.body.size());
                            len = content_length.size();
                            std::memcpy(ptr, content_length.data(), len);
                            ptr += len;
                            remaining -= len;

                            *ptr++ = '\r';
                            *ptr++ = '\n';
                            remaining -= 2;
                        }

                        // End of headers
                        *ptr++ = '\r';
                        *ptr++ = '\n';
                        remaining -= 2;

                        // Copy body
                        if (!request.body.empty()) {
                            len = request.body.size();
                            std::memcpy(ptr, request.body.data(), len);
                            ptr += len;
                            remaining -= len;
                        }

                        // Verify we used exactly the amount of space we calculated
                        assert(remaining == 0 && "Buffer size calculation was incorrect");

                        buffer.resize(buffer.capacity());
                        return buffer;
                    }
                };
                
                // HTTP response string structure
                struct rsp
                {
                    int status_code;
                    
                    std::string status_message;
                    
                    uint8_t major_version;
                    uint8_t minor_version;
                    
                    std::map<std::string, std::string> headers;
                    
                    std::string body;
                    
                    // Constructor with default values
                    inline rsp() 
                        : status_code(200)
                        , status_message("OK")
                        , major_version(1)
                        , minor_version(1) 
                    {
                    }
                    
                    // Constructor with status code
                    inline rsp(int code) 
                        : status_code(code)
                        , status_message(default_status_message(code))
                        , major_version(1)
                        , minor_version(1) 
                    {
                    }
                    
                    // Constructor from in-situ response
                    inline rsp(const rsp_insitu& insitu)
                        : status_code(insitu.status_code)
                        , status_message(insitu.status_message)
                        , major_version(insitu.major_version)
                        , minor_version(insitu.minor_version)
                    {
                        // Copy headers
                        for (const auto& [key, value] : insitu.headers) {
                            headers[std::string(key)] = std::string(value);
                        }
                        
                        // Combine body fragments into a single string
                        if (!insitu.body_fragments.empty()) {
                            size_t total_size = insitu.body_size();
                            body.reserve(total_size);
                            
                            for (const auto& fragment : insitu.body_fragments) {
                                body.append(fragment);
                            }
                        }
                    }
                    
                    // Get default status message for a status code
                    static inline std::string default_status_message(int code) {
                        switch (code) {
                            case 100: return "Continue";
                            case 101: return "Switching Protocols";
                            case 200: return "OK";
                            case 201: return "Created";
                            case 202: return "Accepted";
                            case 204: return "No Content";
                            case 206: return "Partial Content";
                            case 300: return "Multiple Choices";
                            case 301: return "Moved Permanently";
                            case 302: return "Found";
                            case 303: return "See Other";
                            case 304: return "Not Modified";
                            case 307: return "Temporary Redirect";
                            case 308: return "Permanent Redirect";
                            case 400: return "Bad Request";
                            case 401: return "Unauthorized";
                            case 403: return "Forbidden";
                            case 404: return "Not Found";
                            case 405: return "Method Not Allowed";
                            case 406: return "Not Acceptable";
                            case 408: return "Request Timeout";
                            case 409: return "Conflict";
                            case 410: return "Gone";
                            case 411: return "Length Required";
                            case 413: return "Payload Too Large";
                            case 414: return "URI Too Long";
                            case 415: return "Unsupported Media Type";
                            case 416: return "Range Not Satisfiable";
                            case 429: return "Too Many Requests";
                            case 500: return "Internal Server Error";
                            case 501: return "Not Implemented";
                            case 502: return "Bad Gateway";
                            case 503: return "Service Unavailable";
                            case 504: return "Gateway Timeout";
                            case 505: return "HTTP Version Not Supported";
                            default: return "Unknown";
                        }
                    }

                    // Serialize a string-based response to a single buffer
                    io::buf serialize() {
                        auto& response = *this;
                        // Calculate total size needed
                        size_t total_size = 0;

                        // Status line
                        total_size += 5;                         // "HTTP/"
                        total_size += 1;                         // major version
                        total_size += 1;                         // "."
                        total_size += 1;                         // minor version
                        total_size += 1;                         // space

                        std::string status_code_str = std::to_string(response.status_code);
                        total_size += status_code_str.size() + 1;// status code + space

                        total_size += response.status_message.size() + 2; // status message + "\r\n"

                        // Headers
                        for (const auto& [key, value] : response.headers) {
                            total_size += key.size() + 2;        // key + ": "
                            total_size += value.size() + 2;      // value + "\r\n"
                        }

                        // Check if Content-Length header is needed
                        bool has_content_length = false;
                        if (!response.body.empty()) {
                            for (const auto& [key, value] : response.headers) {
                                if (llhttp::case_insensitive_compare(key.c_str(), "Content-Length") == 0) {
                                    has_content_length = true;
                                    break;
                                }
                            }

                            if (!has_content_length) {
                                // Content-Length: + number + \r\n
                                std::string content_length = std::to_string(response.body.size());
                                total_size += 16 + content_length.size() + 2; // "Content-Length: " + size + "\r\n"
                            }
                        }

                        // End of headers line
                        total_size += 2;                         // "\r\n"

                        // Body
                        total_size += response.body.size();

                        // Allocate buffer of the calculated size
                        io::buf buffer(total_size);
                        char* ptr = static_cast<char*>(buffer.data());
                        size_t remaining = total_size;

                        // Copy status line
                        const char* http_ver = "HTTP/";
                        size_t len = 5; // Length of "HTTP/"
                        std::memcpy(ptr, http_ver, len);
                        ptr += len;
                        remaining -= len;

                        *ptr++ = '0' + response.major_version;
                        remaining--;

                        *ptr++ = '.';
                        remaining--;

                        *ptr++ = '0' + response.minor_version;
                        remaining--;

                        *ptr++ = ' ';
                        remaining--;

                        // Status code
                        len = status_code_str.size();
                        std::memcpy(ptr, status_code_str.data(), len);
                        ptr += len;
                        remaining -= len;

                        *ptr++ = ' ';
                        remaining--;

                        // Status message
                        len = response.status_message.size();
                        std::memcpy(ptr, response.status_message.data(), len);
                        ptr += len;
                        remaining -= len;

                        *ptr++ = '\r';
                        *ptr++ = '\n';
                        remaining -= 2;

                        // Copy headers
                        for (const auto& [key, value] : response.headers) {
                            len = key.size();
                            std::memcpy(ptr, key.data(), len);
                            ptr += len;
                            remaining -= len;

                            *ptr++ = ':';
                            *ptr++ = ' ';
                            remaining -= 2;

                            len = value.size();
                            std::memcpy(ptr, value.data(), len);
                            ptr += len;
                            remaining -= len;

                            *ptr++ = '\r';
                            *ptr++ = '\n';
                            remaining -= 2;
                        }

                        // Add Content-Length header if needed
                        if (!response.body.empty() && !has_content_length) {
                            const char* cl_header = "Content-Length: ";
                            len = 16; // Length of "Content-Length: "
                            std::memcpy(ptr, cl_header, len);
                            ptr += len;
                            remaining -= len;

                            std::string content_length = std::to_string(response.body.size());
                            len = content_length.size();
                            std::memcpy(ptr, content_length.data(), len);
                            ptr += len;
                            remaining -= len;

                            *ptr++ = '\r';
                            *ptr++ = '\n';
                            remaining -= 2;
                        }

                        // End of headers
                        *ptr++ = '\r';
                        *ptr++ = '\n';
                        remaining -= 2;

                        // Copy body
                        if (!response.body.empty()) {
                            len = response.body.size();
                            std::memcpy(ptr, response.body.data(), len);
                            ptr += len;
                            remaining -= len;
                        }

                        // Verify we used exactly the amount of space we calculated
                        assert(remaining == 0 && "Buffer size calculation was incorrect");

                        buffer.resize(buffer.capacity());
                        return buffer;
                    }
                };
                
                // HTTP request parser - parses io::buf into req_insitu
                struct req_parser{
                    using prot_output_type = req_insitu;
                    
                    template <typename T_FSM>
                    inline req_parser(fsm<T_FSM>& state_machine) : manager(state_machine.getManager()) {
                        init_parser(&parser);
                        parser.data = this;
                        
                        // Initialize fragment tracking
                        fragment_count = 0;
                        temp_storage.clear();
                    }

                    inline req_parser(io::manager* _manager) : manager(_manager) {
                        init_parser(&parser);
                        parser.data = this;

                        // Initialize fragment tracking
                        fragment_count = 0;
                        temp_storage.clear();
                    }
                    
                    // Output operation - implements output protocol
                    inline void operator>>(future_with<req_insitu>& fut) {
                        current_request.send_prom = manager->make_future(fut, &fut.data);
                        if (request_complete)
                        {
                            current_request.try_send();
                            request_complete = false;
                        }
                    }
                    
                    // Input operation - implements input protocol for io::buf
                    inline future operator<<(io::buf& data) {
                        future fut;
                        current_request.recv_prom = manager->make_future(fut);
                        
                        // Parse the data
                        enum llhttp::llhttp_errno err = llhttp::llhttp_execute(
                            &parser, 
                            static_cast<const char*>(data.data()),
                            data.size()
                        );
                        
                        if (err == llhttp::HPE_OK) {
                            if (buffer_save) {
                                current_request.temp->raw_buffers.push_back(std::move(data));
                                buffer_save = false;
                            }
                            if (request_complete == true) {
                                if (current_request.try_send()) {
                                    llhttp::llhttp_reset(&parser);
                                    current_request.recv_prom.resolve();
                                }
                            }
                            else
                            {
                                current_request.recv_prom.resolve();
                            }
                        } else {
                            current_request.recv_prom.reject(std::make_error_code(std::errc::protocol_error));
                        }
                        
                        return fut;
                    }
                    
                    IO_MANAGER_BAN_COPY(req_parser);
                    // Add explicit move constructor and move assignment operator
                    req_parser(req_parser&& other) noexcept 
                        : current_request(std::move(other.current_request))
                        , manager(other.manager)
                        , buffer_save(other.buffer_save)
                        , temp_string_view(other.temp_string_view)
                        , temp_string_view_header(other.temp_string_view_header)
                        , fragment_count(other.fragment_count)
                        , temp_storage(std::move(other.temp_storage))
                        , request_complete(other.request_complete)
                    {
						init_parser(&parser);
                        parser.data = this;
                    }

                    req_parser& operator=(req_parser&& other) noexcept {
                        if (this != &other) {
                            manager = other.manager;
                            current_request = std::move(other.current_request);
                            buffer_save = other.buffer_save;
                            temp_string_view = other.temp_string_view;
                            temp_string_view_header = other.temp_string_view_header;
                            fragment_count = other.fragment_count;
                            temp_storage = std::move(other.temp_storage);
                            request_complete = other.request_complete;

                            init_parser(&parser);
                            parser.data = this;
                        }
                        return *this;
                    }
                private:
                    protocol_lock<req_insitu> current_request;

                    llhttp::llhttp_t parser;
                    io::manager* manager;
                    
                    bool buffer_save = false;
                    
                    std::string_view temp_string_view;
                    std::string_view temp_string_view_header;
                    int fragment_count = 0;
                    std::string temp_storage;  // Unified temporary storage for header fragments

					bool request_complete = false;

					static void init_parser(llhttp::llhttp_t* parser) {
                        static const llhttp::llhttp_settings_t settings = []() {
                            llhttp::llhttp_settings_t settings;
                            settings.on_message_begin = on_message_begin;
                            settings.on_url = on_url;
                            settings.on_url_complete = on_url_complete;
                            settings.on_header_field = on_header_field;
                            settings.on_header_field_complete = on_header_field_complete;
                            settings.on_header_value = on_header_value;
                            settings.on_header_value_complete = on_header_value_complete;
                            settings.on_body = on_body;
                            settings.on_message_complete = on_message_complete;
                            return settings;
                            }();
						llhttp::llhttp_init(parser, llhttp::HTTP_REQUEST, &settings);
					}
                    
                    // Callback functions for llhttp
                    static int on_message_begin(llhttp::llhttp_t* parser) {
                        auto* self = static_cast<req_parser*>(parser->data);
                        if (!self) return 0;
                        
                        // Reset current request
                        self->current_request.temp = req_insitu{};
                        
                        // Reset fragment tracking
                        self->fragment_count = 0;
                        self->temp_storage.clear();
                        self->temp_string_view = {};
                        self->temp_string_view_header = {};
                        
                        // Reset buffer tracking
                        self->buffer_save = false;
                        
                        return 0;
                    }
                    
                    static int on_url(llhttp::llhttp_t* parser, const char* at, size_t length) {
                        auto* self = static_cast<req_parser*>(parser->data);
                        if (!self) return 0;
                        
                        self->fragment_count++;
                        
                        if (self->fragment_count == 1) {
                            self->temp_string_view = std::string_view(at, length);
                            self->buffer_save = true;
                        } else if (self->fragment_count == 2) {
                            self->temp_storage = std::string(self->temp_string_view);
                            self->temp_storage.append(at, length);
                            self->buffer_save = false;
                        } else {
                            self->temp_storage.append(at, length);
                            self->buffer_save = false;
                        }
                        
                        return 0;
                    }
                    
                    static int on_url_complete(llhttp::llhttp_t* parser) {
                        auto* self = static_cast<req_parser*>(parser->data);
                        if (!self) return 0;
                        
                        if (self->fragment_count > 1) {
                            io::buf url_buf(std::span(self->temp_storage.data(), self->temp_storage.size()));
                            
                            self->current_request.temp->url = std::string_view(
                                static_cast<const char*>(url_buf.data()), 
                                self->temp_storage.size()
                            );
                            
                            self->current_request.temp->raw_buffers.push_back(std::move(url_buf));
                        }
                        else
                        {
							self->current_request.temp->url = self->temp_string_view;
                        }
                        
                        self->fragment_count = 0;
                        self->temp_storage.clear();
                        
                        return 0;
                    }
                    
                    static int on_header_field(llhttp::llhttp_t* parser, const char* at, size_t length) {
                        auto* self = static_cast<req_parser*>(parser->data);
                        if (!self) return 0;

                        self->fragment_count++;

                        if (self->fragment_count == 1) {
                            self->temp_string_view = std::string_view(at, length);
                            self->buffer_save = true;
                        }
                        else if (self->fragment_count == 2) {
                            self->temp_storage = std::string(self->temp_string_view);
                            self->temp_storage.append(at, length);
                            self->buffer_save = false;
                        }
                        else {
                            self->temp_storage.append(at, length);
                            self->buffer_save = false;
                        }

                        return 0;
                    }
                    
                    static int on_header_field_complete(llhttp::llhttp_t* parser) {
                        auto* self = static_cast<req_parser*>(parser->data);
                        if (!self) return 0;

                        if (self->fragment_count > 1) {
                            io::buf url_buf(std::span(self->temp_storage.data(), self->temp_storage.size()));

                            self->temp_string_view_header = std::string_view(
                                static_cast<const char*>(url_buf.data()),
                                self->temp_storage.size()
                            );

                            self->current_request.temp->raw_buffers.push_back(std::move(url_buf));
                        }
                        else
                        {
                            self->temp_string_view_header = self->temp_string_view;
                        }

                        self->fragment_count = 0;
                        self->temp_storage.clear();

                        return 0;
                    }
                    
                    static int on_header_value(llhttp::llhttp_t* parser, const char* at, size_t length) {
                        auto* self = static_cast<req_parser*>(parser->data);
                        if (!self) return 0;

                        self->fragment_count++;

                        if (self->fragment_count == 1) {
                            self->temp_string_view = std::string_view(at, length);
                            self->buffer_save = true;
                        }
                        else if (self->fragment_count == 2) {
                            self->temp_storage = std::string(self->temp_string_view);
                            self->temp_storage.append(at, length);
                            self->buffer_save = false;
                        }
                        else {
                            self->temp_storage.append(at, length);
                            self->buffer_save = false;
                        }

                        return 0;
                    }
                    
                    static int on_header_value_complete(llhttp::llhttp_t* parser) {
                        auto* self = static_cast<req_parser*>(parser->data);
                        if (!self) return 0;

                        if (self->fragment_count > 1) {
                            io::buf url_buf(std::span(self->temp_storage.data(), self->temp_storage.size()));

							self->current_request.temp->headers[self->temp_string_view_header] = std::string_view(
								static_cast<const char*>(url_buf.data()),
								self->temp_storage.size()
							);

                            self->current_request.temp->raw_buffers.push_back(std::move(url_buf));
                        }
                        else
                        {
							self->current_request.temp->headers[self->temp_string_view_header] = self->temp_string_view;
                        }

                        self->fragment_count = 0;
                        self->temp_storage.clear();

                        return 0;
                    }
                    
                    static int on_body(llhttp::llhttp_t* parser, const char* at, size_t length) {
                        auto* self = static_cast<req_parser*>(parser->data);
                        if (!self) return 0;
                        
                        self->current_request.temp->body_fragments.emplace_back(at, length);
                        self->buffer_save = true;
                        
                        return 0;
                    }
                    
                    static int on_message_complete(llhttp::llhttp_t* parser) {
                        auto* self = static_cast<req_parser*>(parser->data);
                        if (!self) return 0;
                        
                        self->current_request.temp->method = static_cast<llhttp::llhttp_method_t>(parser->method);
                        self->current_request.temp->major_version = parser->http_major;
                        self->current_request.temp->minor_version = parser->http_minor;
                        
                        self->request_complete = true;
                        
                        return 0;
                    }
                };

                // HTTP response parser - parses io::buf into rsp_insitu
                struct rsp_parser
                {
                    using prot_output_type = rsp_insitu;

                    template <typename T_FSM>
                    inline rsp_parser(fsm<T_FSM> &state_machine) : manager(state_machine.getManager())
                    {
                        init_parser(&parser);
                        parser.data = this;

                        // Initialize fragment tracking
                        fragment_count = 0;
                        temp_storage.clear();
                    }

                    inline rsp_parser(io::manager* _manager) : manager(_manager)
                    {
                        init_parser(&parser);
                        parser.data = this;

                        // Initialize fragment tracking
                        fragment_count = 0;
                        temp_storage.clear();
                    }

                    // Output operation - implements output protocol
                    inline void operator>>(future_with<rsp_insitu> &fut)
                    {
                        current_response.send_prom = manager->make_future(fut, &fut.data);
                        if (response_complete)
                        {
                            current_response.try_send();
                            response_complete = false;
                        }
                    }

                    // Input operation - implements input protocol for io::buf
                    inline io::future operator<<(io::buf &data)
                    {
                        future fut;
                        current_response.recv_prom = manager->make_future(fut);

                        // Parse the data
                        enum llhttp::llhttp_errno err = llhttp::llhttp_execute(
                            &parser,
                            static_cast<const char *>(data.data()),
                            data.size());

                        if (err == llhttp::HPE_OK)
                        {
                            // Data parsed successfully, handle the buffer
                            // We only keep the buffer if it's part of the result (contains string_views)
                            if (buffer_save)
                            {
                                current_response.temp->raw_buffers.push_back(std::move(data));
                                buffer_save = false;
                            }
                            if (response_complete == true)
                            {
                                if (current_response.try_send()) {
                                    llhttp::llhttp_reset(&parser);
                                    current_response.recv_prom.resolve();
                                }
                            }
                            else
                            {
                                current_response.recv_prom.resolve();
                            }
                        }
                        else
                        {
                            // Parsing error
                            current_response.recv_prom.reject(std::make_error_code(std::errc::protocol_error));
                        }

                        return fut;
                    }

                    IO_MANAGER_BAN_COPY(rsp_parser);
                    // Add explicit move constructor and move assignment operator
                    rsp_parser(rsp_parser&& other) noexcept 
                        : current_response(std::move(other.current_response))
                        , manager(other.manager)
                        , buffer_save(other.buffer_save)
                        , temp_string_view(other.temp_string_view)
                        , temp_string_view_header(other.temp_string_view_header)
                        , fragment_count(other.fragment_count)
                        , temp_storage(std::move(other.temp_storage))
                        , saved_buffer(std::move(other.saved_buffer))
                        , response_complete(other.response_complete)
                    {
                        init_parser(&parser);
                        parser.data = this;
                    }

                    rsp_parser& operator=(rsp_parser&& other) noexcept {
                        if (this != &other) {
                            manager = other.manager;
                            current_response = std::move(other.current_response);
                            buffer_save = other.buffer_save;
                            temp_string_view = other.temp_string_view;
                            temp_string_view_header = other.temp_string_view_header;
                            fragment_count = other.fragment_count;
                            temp_storage = std::move(other.temp_storage);
                            saved_buffer = std::move(other.saved_buffer);
                            response_complete = other.response_complete;
                            
                            init_parser(&parser);
                            parser.data = this;
                        }
                        return *this;
                    }
                private:
                    protocol_lock<rsp_insitu> current_response;

                    llhttp::llhttp_t parser;
                    io::manager *manager;

                    bool buffer_save = false;

                    // Simplified fragment handling - we only process one fragment type at a time
                    std::string_view temp_string_view;
                    std::string_view temp_string_view_header;
                    int fragment_count = 0;
                    std::string temp_storage; // Unified temporary storage for fragments
                    io::buf saved_buffer;     // Buffer that contains the first fragment

                    // State tracking
                    bool response_complete = false; // Indicates if a complete response is waiting

                    static void init_parser(llhttp::llhttp_t* parser) {
                        static const llhttp::llhttp_settings_t settings = []() {
                            llhttp::llhttp_settings_t settings;
                            llhttp::llhttp_settings_init(&settings);
                            // Set callback functions
                            settings.on_message_begin = on_message_begin;
                            settings.on_status = on_status;
                            settings.on_status_complete = on_status_complete;
                            settings.on_header_field = on_header_field;
                            settings.on_header_field_complete = on_header_field_complete;
                            settings.on_header_value = on_header_value;
                            settings.on_header_value_complete = on_header_value_complete;
                            settings.on_body = on_body;
                            settings.on_message_complete = on_message_complete;
                            return settings;
                        }();
                        llhttp::llhttp_init(parser, llhttp::HTTP_RESPONSE, &settings);
                    }

                    // Callback functions for llhttp
                    static int on_message_begin(llhttp::llhttp_t *parser)
                    {
                        auto *self = static_cast<rsp_parser *>(parser->data);
                        if (!self)
                            return 0;

                        // Reset current response
                        self->current_response.temp = rsp_insitu{};

                        // Reset fragment tracking
                        self->fragment_count = 0;
                        self->temp_storage.clear();
                        self->temp_string_view = {};
                        self->temp_string_view_header = {};
                        self->saved_buffer = {};

                        // Reset buffer tracking
                        self->buffer_save = false;
                        
                        // Reset completion flag
                        self->response_complete = false;

                        return 0;
                    }

                    static int on_status(llhttp::llhttp_t *parser, const char *at, size_t length)
                    {
                        auto *self = static_cast<rsp_parser *>(parser->data);
                        if (!self)
                            return 0;

                        self->fragment_count++;

                        if (self->fragment_count == 1)
                        {
                            self->temp_string_view = std::string_view(at, length);
                            self->buffer_save = true;
                        }
                        else if (self->fragment_count == 2)
                        {
                            self->temp_storage = std::string(self->temp_string_view);
                            self->temp_storage.append(at, length);
                            self->buffer_save = false;
                        }
                        else
                        {
                            self->temp_storage.append(at, length);
                            self->buffer_save = false;
                        }

                        return 0;
                    }

                    static int on_status_complete(llhttp::llhttp_t *parser)
                    {
                        auto *self = static_cast<rsp_parser *>(parser->data);
                        if (!self)
                            return 0;

                        if (self->fragment_count > 1)
                        {
                            io::buf status_buf(std::span(self->temp_storage.data(), self->temp_storage.size()));

                            self->current_response.temp->status_message = std::string_view(
                                static_cast<const char *>(status_buf.data()),
                                self->temp_storage.size());

                            self->current_response.temp->raw_buffers.push_back(std::move(status_buf));
                        }
                        else
                        {
                            self->current_response.temp->status_message = self->temp_string_view;
                        }

                        self->fragment_count = 0;
                        self->temp_storage.clear();

                        return 0;
                    }

                    static int on_header_field(llhttp::llhttp_t *parser, const char *at, size_t length)
                    {
                        auto *self = static_cast<rsp_parser *>(parser->data);
                        if (!self)
                            return 0;

                        self->fragment_count++;

                        if (self->fragment_count == 1)
                        {
                            self->temp_string_view = std::string_view(at, length);
                            self->buffer_save = true;
                        }
                        else if (self->fragment_count == 2)
                        {
                            self->temp_storage = std::string(self->temp_string_view);
                            self->temp_storage.append(at, length);
                            self->buffer_save = false;
                        }
                        else
                        {
                            self->temp_storage.append(at, length);
                            self->buffer_save = false;
                        }

                        return 0;
                    }

                    static int on_header_field_complete(llhttp::llhttp_t *parser)
                    {
                        auto *self = static_cast<rsp_parser *>(parser->data);
                        if (!self)
                            return 0;

                        if (self->fragment_count > 1)
                        {
                            io::buf header_buf(std::span(self->temp_storage.data(), self->temp_storage.size()));

                            self->temp_string_view_header = std::string_view(
                                static_cast<const char *>(header_buf.data()),
                                self->temp_storage.size());

                            self->current_response.temp->raw_buffers.push_back(std::move(header_buf));
                        }
                        else
                        {
                            self->temp_string_view_header = self->temp_string_view;
                        }

                        self->fragment_count = 0;
                        self->temp_storage.clear();

                        return 0;
                    }

                    static int on_header_value(llhttp::llhttp_t *parser, const char *at, size_t length)
                    {
                        auto *self = static_cast<rsp_parser *>(parser->data);
                        if (!self)
                            return 0;

                        self->fragment_count++;

                        if (self->fragment_count == 1)
                        {
                            self->temp_string_view = std::string_view(at, length);
                            self->buffer_save = true;
                        }
                        else if (self->fragment_count == 2)
                        {
                            self->temp_storage = std::string(self->temp_string_view);
                            self->temp_storage.append(at, length);
                            self->buffer_save = false;
                        }
                        else
                        {
                            self->temp_storage.append(at, length);
                            self->buffer_save = false;
                        }

                        return 0;
                    }

                    static int on_header_value_complete(llhttp::llhttp_t *parser)
                    {
                        auto *self = static_cast<rsp_parser *>(parser->data);
                        if (!self)
                            return 0;

                        if (self->fragment_count > 1)
                        {
                            io::buf header_value_buf(std::span(self->temp_storage.data(), self->temp_storage.size()));

                            self->current_response.temp->headers[self->temp_string_view_header] = std::string_view(
                                static_cast<const char *>(header_value_buf.data()),
                                self->temp_storage.size());

                            self->current_response.temp->raw_buffers.push_back(std::move(header_value_buf));
                        }
                        else
                        {
                            self->current_response.temp->headers[self->temp_string_view_header] = self->temp_string_view;
                        }

                        self->fragment_count = 0;
                        self->temp_storage.clear();

                        return 0;
                    }

                    static int on_body(llhttp::llhttp_t *parser, const char *at, size_t length)
                    {
                        auto *self = static_cast<rsp_parser *>(parser->data);
                        if (!self)
                            return 0;

                        self->current_response.temp->body_fragments.emplace_back(at, length);
                        self->buffer_save = true;

                        return 0;
                    }

                    static int on_message_complete(llhttp::llhttp_t *parser)
                    {
                        auto *self = static_cast<rsp_parser *>(parser->data);
                        if (!self)
                            return 0;

                        // Status code is already set by llhttp
                        self->current_response.temp->status_code = parser->status_code;
                        self->current_response.temp->major_version = parser->http_major;
                        self->current_response.temp->minor_version = parser->http_minor;

                        self->response_complete = true;

                        return 0;
                    }
                };

                // Unified HTTP serializer - serializes any HTTP message type into io::buf to send
                struct serializer {
                    using prot_output_type = io::buf;
                    
                    template <typename T_FSM>
                    inline serializer(fsm<T_FSM>& state_machine) : manager(state_machine.getManager()) {}

                    inline serializer(io::manager* _manager) : manager(_manager) {}
                    
                    // Output operation - implements output protocol
                    inline void operator>>(future_with<io::buf>& fut) {
                        // Will resolve this future when we have a buffer to send
                        pending_promise = manager->make_future(fut, &fut.data);
                        deliver_buffer();
                    }
                    
                    // Input operations for request types
                    inline future operator<<(req& request) {
                        future fut;
                        pending_input_promise = manager->make_future(fut);

                        // Check if previous output has been sent
                        if (buffers.size()) {
                            return fut;
                        }
                        
                        // Serialize string-based request to a single buffer
                        io::buf combined_buffer = request.serialize();
                        buffers.push_back(std::move(combined_buffer));

                        deliver_buffer();
                        return fut;
                    }
                    
                    inline future operator<<(req_insitu& request) {
                        future fut;
                        pending_input_promise = manager->make_future(fut);
                        
                        // Check if previous output has been sent
                        if (buffers.size()) {
                            return fut;
                        }
                        
                        // For in-situ request, we can reuse existing buffers for better performance
                        if (!request.raw_buffers.empty()) {
                            // Move raw buffers from request to our buffer queue
                            for (auto& buf : request.raw_buffers) {
                                buffers.push_back(std::move(buf));
                            }
                        }

                        deliver_buffer();
                        return fut;
                    }
                    
                    // Input operations for response types
                    inline future operator<<(rsp& response) {
                        future fut;
                        pending_input_promise = manager->make_future(fut);
                        
                        // Check if previous output has been sent
                        if (buffers.size()) {
                            return fut;
                        }
                        
                        // Serialize string-based response to a single buffer
                        io::buf combined_buffer = response.serialize();
                        buffers.push_back(std::move(combined_buffer));

                        deliver_buffer();
                        return fut;
                    }
                    
                    inline future operator<<(rsp_insitu& response) {
                        future fut;
                        pending_input_promise = manager->make_future(fut);

                        // Check if previous output has been sent
                        if (buffers.size()) {
                            return fut;
                        }
                        
                        // For in-situ response, we can reuse existing buffers for better performance
                        if (!response.raw_buffers.empty()) {
                            // Move raw buffers from response to our buffer queue
                            for (auto& buf : response.raw_buffers) {
                                buffers.push_back(std::move(buf));
                            }
                        }

                        deliver_buffer();
                        return fut;
                    }
                    
                    serializer(serializer&) = delete;
                    serializer& operator=(serializer&) = delete;

                    serializer(serializer&&) = default;
                    serializer& operator=(serializer&&) = default;
                private:
                    io::promise<io::buf> pending_promise;
                    io::promise<void> pending_input_promise;  // For blocked input operations
                    io::manager* manager;
                    
                    std::vector<io::buf> buffers;  // Queue of buffers to send
                    
                    // Deliver the next buffer to the pending output, if any
                    void deliver_buffer() {
                        if (pending_promise.valid() && !buffers.empty()) {
                            // Move the first buffer to the promise
                            *pending_promise.data() = std::move(*(buffers.end() - 1));
                            buffers.pop_back();
                            
                            pending_promise.resolve_later();
                            
                            // If the buffer queue is empty, reset the output state
                            if (buffers.empty()) {
                                pending_input_promise.resolve_later();
                            }
                        }
                    }
                };
            };
        };
    };
};

#include "llhttp/api.c"