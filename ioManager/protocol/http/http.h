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
int case_insensitive_compare(const char* s1, const char* s2) {
#ifdef _MSC_VER
                    return _stricmp(s1, s2);
#else
                    return strcasecmp(s1, s2);
#endif
                    }
                };
                
                // HTTP request structure
                struct req
                {
                    // HTTP method (GET, POST, etc.)
                    llhttp::llhttp_method_t method;
                    
                    // URL from the request
                    std::string url;
                    
                    // HTTP version
                    uint8_t major_version;
                    uint8_t minor_version;
                    
                    // Headers storage
                    std::unordered_map<std::string, std::string> headers;
                    
                    // Request body
                    std::string body;
                    
                    // Get header value by name (case-insensitive)
                    inline std::string_view get_header(const std::string& name) const {
                        for (const auto& [key, value] : headers) {
                            if (llhttp::case_insensitive_compare(key.c_str(), name.c_str()) == 0) {
                                return value;
                            }
                        }
                        return {};
                    }
                    
                    // Check if a header exists
                    inline bool has_header(const std::string& name) const {
                        return !get_header(name).empty();
                    }
                    
                    // Get content length
                    inline size_t content_length() const {
                        auto cl = get_header("Content-Length");
                        if (cl.empty()) return 0;
                        return std::stoul(std::string(cl));
                    }
                    
                    // Get content type
                    inline std::string_view content_type() const {
                        return get_header("Content-Type");
                    }
                    
                    // Convert method to string
                    inline std::string_view method_name() const {
                        return llhttp::llhttp_method_name(method);
                    }
                };
                
                // HTTP response structure
                struct rsp
                {
                    // HTTP status code
                    int status_code;
                    
                    // Status message
                    std::string status_message;
                    
                    // HTTP version
                    uint8_t major_version;
                    uint8_t minor_version;
                    
                    // Headers storage
                    std::unordered_map<std::string, std::string> headers;
                    
                    // Response body
                    std::string body;
                    
                    // Constructor with default values
                    inline rsp() : status_code(200), major_version(1), minor_version(1) {}
                    
                    // Constructor with status code
                    inline rsp(int code) : status_code(code), major_version(1), minor_version(1) {
                        status_message = default_status_message(code);
                    }
                    
                    // Get header value by name (case-insensitive)
                    inline std::string_view get_header(const std::string& name) const {
                        for (const auto& [key, value] : headers) {
                            if (llhttp::case_insensitive_compare(key.c_str(), name.c_str()) == 0) {
                                return value;
                            }
                        }
                        return {};
                    }
                    
                    // Check if a header exists
                    inline bool has_header(const std::string& name) const {
                        return !get_header(name).empty();
                    }
                    
                    // Set a header
                    inline void set_header(const std::string& name, const std::string& value) {
                        headers[name] = value;
                    }
                    
                    // Get content length
                    inline size_t content_length() const {
                        auto cl = get_header("Content-Length");
                        if (cl.empty()) return 0;
                        return std::stoul(std::string(cl));
                    }
                    
                    // Get content type
                    inline std::string_view content_type() const {
                        return get_header("Content-Type");
                    }
                    
                    // Set content type
                    inline void set_content_type(const std::string& type) {
                        set_header("Content-Type", type);
                    }
                    
                    // Set content length
                    inline void set_content_length(size_t length) {
                        set_header("Content-Length", std::to_string(length));
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
                    
                    // Serialize response to string
                    inline std::string to_string() const {
                        std::stringstream ss;
                        
                        // Status line
                        ss << "HTTP/" << (int)major_version << "." << (int)minor_version 
                           << " " << status_code << " " << status_message << "\r\n";
                        
                        // Headers
                        for (const auto& [name, value] : headers) {
                            ss << name << ": " << value << "\r\n";
                        }
                        
                        // Empty line separating headers from body
                        ss << "\r\n";
                        
                        // Body
                        ss << body;
                        
                        return ss.str();
                    }
                };
                
                struct req_parser{
                    __IO_INTERNAL_HEADER_PERMISSION;
                    using prot_output_type = req;
                    using prot_input_type = std::span<char>;
                    
                    template <typename T_FSM>
                    inline req_parser(fsm<T_FSM>& state_machine) : manager(state_machine.getManager()) {
                        llhttp::llhttp_settings_t settings;
                        llhttp::llhttp_settings_init(&settings);
                        
                        // Set callback functions
                        settings.on_message_begin = on_message_begin;
                        settings.on_url = on_url;
                        settings.on_header_field = on_header_field;
                        settings.on_header_value = on_header_value;
                        settings.on_body = on_body;
                        settings.on_message_complete = on_message_complete;
                        
                        llhttp::llhttp_init(&parser, llhttp::HTTP_REQUEST, &settings);
                        parser.data = this;
                    }
                    
                    // Output operation - implements output protocol
                    inline void operator>>(future_with<req>& fut) {
                        // Will resolve this future when a complete HTTP request is parsed
                        io::promise<req> promise = manager->make_future(fut, &fut.data);
                        // Store promise to resolve it when parsing completes
                        pending_promise = std::move(promise);
                    }
                    
                    // Input operation - implements input protocol
                    inline future operator<<(const std::span<char>& data) {
                        future fut;
                        io::promise<void> promise = manager->make_future(fut);
                        
                        // Parse received data
                        enum llhttp::llhttp_errno err = llhttp::llhttp_execute(&parser, data.data(), data.size());
                        
                        if (err == llhttp::HPE_OK) {
                            // Data parsed successfully
                            promise.resolve();
                        } else if (err == llhttp::HPE_PAUSED) {
                            // Request complete, callback triggered
                            llhttp::llhttp_resume(&parser);
                            promise.resolve();
                        } else {
                            // Parsing error
                            promise.reject(std::make_error_code(std::errc::protocol_error));
                        }
                        
                        return fut;
                    }
                    
                    IO_MANAGER_BAN_COPY(req_parser);
                private:
                    io::promise<req> pending_promise;
                    
                    llhttp::llhttp_t parser;
                    io::manager* manager;
                    req current_request;
                    std::string current_header_field;
                    
                    // Callback functions for llhttp
                    static int on_message_begin(llhttp::llhttp_t* parser) {
                        auto* self = static_cast<req_parser*>(parser->data);
                        if (!self) return 0;
                        
                        // Reset current request
                        self->current_request = req{};
                        return 0;
                    }
                    
                    static int on_url(llhttp::llhttp_t* parser, const char* at, size_t length) {
                        auto* self = static_cast<req_parser*>(parser->data);
                        if (!self) return 0;
                        
                        self->current_request.url.append(at, length);
                        return 0;
                    }
                    
                    static int on_header_field(llhttp::llhttp_t* parser, const char* at, size_t length) {
                        auto* self = static_cast<req_parser*>(parser->data);
                        if (!self) return 0;
                        
                        self->current_header_field.append(at, length);
                        return 0;
                    }
                    
                    static int on_header_value(llhttp::llhttp_t* parser, const char* at, size_t length) {
                        auto* self = static_cast<req_parser*>(parser->data);
                        if (!self) return 0;
                        
                        self->current_request.headers[self->current_header_field].append(at, length);
                        self->current_header_field.clear();
                        return 0;
                    }
                    
                    static int on_body(llhttp::llhttp_t* parser, const char* at, size_t length) {
                        auto* self = static_cast<req_parser*>(parser->data);
                        if (!self) return 0;
                        
                        self->current_request.body.append(at, length);
                        return 0;
                    }
                    
                    static int on_message_complete(llhttp::llhttp_t* parser) {
                        auto* self = static_cast<req_parser*>(parser->data);
                        if (!self) return 0;
                        
                        // Set method and version
                        self->current_request.method = static_cast<llhttp::llhttp_method_t>(parser->method);
                        self->current_request.major_version = parser->http_major;
                        self->current_request.minor_version = parser->http_minor;
                        
                        // Resolve the promise with the completed request
                        if (self->pending_promise.valid()) {
                            *self->pending_promise.data() = self->current_request;
                            self->pending_promise.resolve();
                        }
                        
                        // Pause parser to prevent further processing until next input
                        llhttp::llhttp_pause(parser);
                        return 0;
                    }
                };
                
                struct rsp_parser{
                    __IO_INTERNAL_HEADER_PERMISSION;
                    using prot_output_type = rsp;
                    using prot_input_type = std::span<char>;
                    
                    template <typename T_FSM>
                    inline rsp_parser(fsm<T_FSM>& state_machine) : manager(state_machine.getManager()) {
                        llhttp::llhttp_settings_t settings;
                        llhttp::llhttp_settings_init(&settings);
                        
                        // Set callback functions
                        settings.on_message_begin = on_message_begin;
                        settings.on_status = on_status;
                        settings.on_header_field = on_header_field;
                        settings.on_header_value = on_header_value;
                        settings.on_body = on_body;
                        settings.on_message_complete = on_message_complete;
                        
                        llhttp::llhttp_init(&parser, llhttp::HTTP_RESPONSE, &settings);
                        parser.data = this;
                    }
                    
                    // Output operation - implements output protocol
                    inline void operator>>(future_with<rsp>& fut) {
                        // Will resolve this future when a complete HTTP response is parsed
                        io::promise<rsp> promise = manager->make_future(fut, &fut.data);
                        // Store promise to resolve it when parsing completes
                        pending_promise = std::move(promise);
                    }
                    
                    // Input operation - implements input protocol
                    inline future operator<<(const std::span<char>& data) {
                        future fut;
                        io::promise<void> promise = manager->make_future(fut);
                        
                        // Parse received data
                        enum llhttp::llhttp_errno err = llhttp::llhttp_execute(&parser, data.data(), data.size());
                        
                        if (err == llhttp::HPE_OK) {
                            // Data parsed successfully
                            promise.resolve();
                        } else if (err == llhttp::HPE_PAUSED) {
                            // Response complete, callback triggered
                            llhttp::llhttp_resume(&parser);
                            promise.resolve();
                        } else {
                            // Parsing error
                            promise.reject(std::make_error_code(std::errc::protocol_error));
                        }
                        
                        return fut;
                    }

                    IO_MANAGER_BAN_COPY(rsp_parser);
                private:
                    io::promise<rsp> pending_promise;
                    
                    llhttp::llhttp_t parser;
                    io::manager* manager;
                    rsp current_response;
                    std::string current_header_field;
                    
                    // Callback functions for llhttp
                    static int on_message_begin(llhttp::llhttp_t* parser) {
                        auto* self = static_cast<rsp_parser*>(parser->data);
                        if (!self) return 0;
                        
                        // Reset current response
                        self->current_response = rsp{};
                        return 0;
                    }
                    
                    static int on_status(llhttp::llhttp_t* parser, const char* at, size_t length) {
                        auto* self = static_cast<rsp_parser*>(parser->data);
                        if (!self) return 0;
                        
                        self->current_response.status_message.append(at, length);
                        return 0;
                    }
                    
                    static int on_header_field(llhttp::llhttp_t* parser, const char* at, size_t length) {
                        auto* self = static_cast<rsp_parser*>(parser->data);
                        if (!self) return 0;
                        
                        self->current_header_field.append(at, length);
                        return 0;
                    }
                    
                    static int on_header_value(llhttp::llhttp_t* parser, const char* at, size_t length) {
                        auto* self = static_cast<rsp_parser*>(parser->data);
                        if (!self) return 0;
                        
                        self->current_response.headers[self->current_header_field].append(at, length);
                        self->current_header_field.clear();
                        return 0;
                    }
                    
                    static int on_body(llhttp::llhttp_t* parser, const char* at, size_t length) {
                        auto* self = static_cast<rsp_parser*>(parser->data);
                        if (!self) return 0;
                        
                        self->current_response.body.append(at, length);
                        return 0;
                    }
                    
                    static int on_message_complete(llhttp::llhttp_t* parser) {
                        auto* self = static_cast<rsp_parser*>(parser->data);
                        if (!self) return 0;
                        
                        // Set status code and version
                        self->current_response.status_code = parser->status_code;
                        self->current_response.major_version = parser->http_major;
                        self->current_response.minor_version = parser->http_minor;
                        
                        // Resolve the promise with the completed response
                        if (self->pending_promise.valid()) {
                            *self->pending_promise.data() = self->current_response;
                            self->pending_promise.resolve();
                        }
                        
                        // Pause parser to prevent further processing until next input
                        llhttp::llhttp_pause(parser);
                        return 0;
                    }
                };
            };
        };
    };
};

#include "llhttp/api.c"