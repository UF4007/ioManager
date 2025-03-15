#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "llhttp.h"

#define CALLBACK_MAYBE(PARSER, NAME)                                          \
  do {                                                                        \
    const llhttp_settings_t* settings;                                        \
    settings = (const llhttp_settings_t*) (PARSER)->settings;                 \
    if (settings == NULL || settings->NAME == NULL) {                         \
      err = 0;                                                                \
      break;                                                                  \
    }                                                                         \
    err = settings->NAME((PARSER));                                           \
  } while (0)

#define SPAN_CALLBACK_MAYBE(PARSER, NAME, START, LEN)                         \
  do {                                                                        \
    const llhttp_settings_t* settings;                                        \
    settings = (const llhttp_settings_t*) (PARSER)->settings;                 \
    if (settings == NULL || settings->NAME == NULL) {                         \
      err = 0;                                                                \
      break;                                                                  \
    }                                                                         \
    err = settings->NAME((PARSER), (const char*)(START), (LEN));                           \
    if (err == -1) {                                                          \
      err = HPE_USER;                                                         \
      llhttp_set_error_reason((PARSER), "Span callback error in " #NAME);     \
    }                                                                         \
  } while (0)

inline void io::prot::http::llhttp::llhttp_init(llhttp_t* parser, llhttp_type_t type,
                 const llhttp_settings_t* settings) {
  llhttp__internal_init(parser);

  parser->type = type;
  parser->settings = (void*) settings;
}


#if defined(__wasm__)

extern int wasm_on_message_begin(llhttp_t * p);
extern int wasm_on_url(llhttp_t* p, const char* at, size_t length);
extern int wasm_on_status(llhttp_t* p, const char* at, size_t length);
extern int wasm_on_header_field(llhttp_t* p, const char* at, size_t length);
extern int wasm_on_header_value(llhttp_t* p, const char* at, size_t length);
extern int wasm_on_headers_complete(llhttp_t * p, int status_code,
                                    uint8_t upgrade, int should_keep_alive);
extern int wasm_on_body(llhttp_t* p, const char* at, size_t length);
extern int wasm_on_message_complete(llhttp_t * p);

static int wasm_on_headers_complete_wrap(llhttp_t* p) {
  return wasm_on_headers_complete(p, p->status_code, p->upgrade,
                                  llhttp_should_keep_alive(p));
}

const llhttp_settings_t wasm_settings = {
  wasm_on_message_begin,
  wasm_on_url,
  wasm_on_status,
  NULL,
  NULL,
  wasm_on_header_field,
  wasm_on_header_value,
  NULL,
  NULL,
  wasm_on_headers_complete_wrap,
  wasm_on_body,
  wasm_on_message_complete,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
};


llhttp_t* llhttp_alloc(llhttp_type_t type) {
  llhttp_t* parser = malloc(sizeof(llhttp_t));
  llhttp_init(parser, type, &wasm_settings);
  return parser;
}

void llhttp_free(llhttp_t* parser) {
  free(parser);
}

#endif  // defined(__wasm__)

/* Some getters required to get stuff from the parser */

inline uint8_t io::prot::http::llhttp::llhttp_get_type(llhttp_t* parser) {
  return parser->type;
}

inline uint8_t io::prot::http::llhttp::llhttp_get_http_major(llhttp_t* parser) {
  return parser->http_major;
}

inline uint8_t io::prot::http::llhttp::llhttp_get_http_minor(llhttp_t* parser) {
  return parser->http_minor;
}

inline uint8_t io::prot::http::llhttp::llhttp_get_method(llhttp_t* parser) {
  return parser->method;
}

inline int io::prot::http::llhttp::llhttp_get_status_code(llhttp_t* parser) {
  return parser->status_code;
}

inline uint8_t io::prot::http::llhttp::llhttp_get_upgrade(llhttp_t* parser) {
  return parser->upgrade;
}


inline void io::prot::http::llhttp::llhttp_reset(llhttp_t* parser) {
  llhttp_type_t type = (llhttp_type_t)parser->type;
  const llhttp_settings_t* settings = (const llhttp_settings_t*)parser->settings;
  void* data = parser->data;
  uint16_t lenient_flags = parser->lenient_flags;

  llhttp__internal_init(parser);

  parser->type = type;
  parser->settings = (void*) settings;
  parser->data = data;
  parser->lenient_flags = lenient_flags;
}


inline io::prot::http::llhttp::llhttp_errno_t io::prot::http::llhttp::llhttp_execute(llhttp_t* parser, const char* data, size_t len) {
  return (llhttp_errno_t)llhttp__internal_execute(parser, data, data + len);
}


inline void io::prot::http::llhttp::llhttp_settings_init(llhttp_settings_t* settings) {
  memset(settings, 0, sizeof(*settings));
}


inline io::prot::http::llhttp::llhttp_errno_t io::prot::http::llhttp::llhttp_finish(llhttp_t* parser) {
  int err;

  /* We're in an error state. Don't bother doing anything. */
  if (parser->error != 0) {
    return (llhttp_errno_t)0;
  }

  switch (parser->finish) {
    case HTTP_FINISH_SAFE_WITH_CB:
      CALLBACK_MAYBE(parser, on_message_complete);
      if (err != HPE_OK) return (llhttp_errno_t)err;

    /* FALLTHROUGH */
    case HTTP_FINISH_SAFE:
      return HPE_OK;
    case HTTP_FINISH_UNSAFE:
      parser->reason = "Invalid EOF state";
      return HPE_INVALID_EOF_STATE;
    default:
      abort();
  }
}


inline void io::prot::http::llhttp::llhttp_pause(llhttp_t* parser) {
  if (parser->error != HPE_OK) {
    return;
  }

  parser->error = HPE_PAUSED;
  parser->reason = "Paused";
}


inline void io::prot::http::llhttp::llhttp_resume(llhttp_t* parser) {
  if (parser->error != HPE_PAUSED) {
    return;
  }

  parser->error = 0;
}


inline void io::prot::http::llhttp::llhttp_resume_after_upgrade(llhttp_t* parser) {
  if (parser->error != HPE_PAUSED_UPGRADE) {
    return;
  }

  parser->error = 0;
}


inline io::prot::http::llhttp::llhttp_errno_t io::prot::http::llhttp::llhttp_get_errno(const llhttp_t* parser) {
  return (llhttp_errno_t)parser->error;
}


inline const char* io::prot::http::llhttp::llhttp_get_error_reason(const llhttp_t* parser) {
  return parser->reason;
}


inline void io::prot::http::llhttp::llhttp_set_error_reason(llhttp_t* parser, const char* reason) {
  parser->reason = reason;
}


inline const char* io::prot::http::llhttp::llhttp_get_error_pos(const llhttp_t* parser) {
  return parser->error_pos;
}


inline const char* io::prot::http::llhttp::llhttp_errno_name(llhttp_errno_t err) {
#define HTTP_ERRNO_GEN(CODE, NAME, _) case HPE_##NAME: return "HPE_" #NAME;
  switch (err) {
    HTTP_ERRNO_MAP(HTTP_ERRNO_GEN)
    default: abort();
  }
#undef HTTP_ERRNO_GEN
}


inline const char* io::prot::http::llhttp::llhttp_method_name(llhttp_method_t method) {
#define HTTP_METHOD_GEN(NUM, NAME, STRING) case HTTP_##NAME: return #STRING;
  switch (method) {
    HTTP_ALL_METHOD_MAP(HTTP_METHOD_GEN)
    default: abort();
  }
#undef HTTP_METHOD_GEN
}

inline const char* io::prot::http::llhttp::llhttp_status_name(llhttp_status_t status) {
#define HTTP_STATUS_GEN(NUM, NAME, STRING) case HTTP_STATUS_##NAME: return #STRING;
  switch (status) {
    HTTP_STATUS_MAP(HTTP_STATUS_GEN)
    default: abort();
  }
#undef HTTP_STATUS_GEN
}


inline void io::prot::http::llhttp::llhttp_set_lenient_headers(llhttp_t* parser, int enabled) {
  if (enabled) {
    parser->lenient_flags |= LENIENT_HEADERS;
  } else {
    parser->lenient_flags &= ~LENIENT_HEADERS;
  }
}


inline void io::prot::http::llhttp::llhttp_set_lenient_chunked_length(llhttp_t* parser, int enabled) {
  if (enabled) {
    parser->lenient_flags |= LENIENT_CHUNKED_LENGTH;
  } else {
    parser->lenient_flags &= ~LENIENT_CHUNKED_LENGTH;
  }
}


inline void io::prot::http::llhttp::llhttp_set_lenient_keep_alive(llhttp_t* parser, int enabled) {
  if (enabled) {
    parser->lenient_flags |= LENIENT_KEEP_ALIVE;
  } else {
    parser->lenient_flags &= ~LENIENT_KEEP_ALIVE;
  }
}

inline void io::prot::http::llhttp::llhttp_set_lenient_transfer_encoding(llhttp_t* parser, int enabled) {
  if (enabled) {
    parser->lenient_flags |= LENIENT_TRANSFER_ENCODING;
  } else {
    parser->lenient_flags &= ~LENIENT_TRANSFER_ENCODING;
  }
}

inline void io::prot::http::llhttp::llhttp_set_lenient_version(llhttp_t* parser, int enabled) {
  if (enabled) {
    parser->lenient_flags |= LENIENT_VERSION;
  } else {
    parser->lenient_flags &= ~LENIENT_VERSION;
  }
}

inline void io::prot::http::llhttp::llhttp_set_lenient_data_after_close(llhttp_t* parser, int enabled) {
  if (enabled) {
    parser->lenient_flags |= LENIENT_DATA_AFTER_CLOSE;
  } else {
    parser->lenient_flags &= ~LENIENT_DATA_AFTER_CLOSE;
  }
}

inline void io::prot::http::llhttp::llhttp_set_lenient_optional_lf_after_cr(llhttp_t* parser, int enabled) {
  if (enabled) {
    parser->lenient_flags |= LENIENT_OPTIONAL_LF_AFTER_CR;
  } else {
    parser->lenient_flags &= ~LENIENT_OPTIONAL_LF_AFTER_CR;
  }
}

inline void io::prot::http::llhttp::llhttp_set_lenient_optional_crlf_after_chunk(llhttp_t* parser, int enabled) {
  if (enabled) {
    parser->lenient_flags |= LENIENT_OPTIONAL_CRLF_AFTER_CHUNK;
  } else {
    parser->lenient_flags &= ~LENIENT_OPTIONAL_CRLF_AFTER_CHUNK;
  }
}

inline void io::prot::http::llhttp::llhttp_set_lenient_optional_cr_before_lf(llhttp_t* parser, int enabled) {
  if (enabled) {
    parser->lenient_flags |= LENIENT_OPTIONAL_CR_BEFORE_LF;
  } else {
    parser->lenient_flags &= ~LENIENT_OPTIONAL_CR_BEFORE_LF;
  }
}

inline void io::prot::http::llhttp::llhttp_set_lenient_spaces_after_chunk_size(llhttp_t* parser, int enabled) {
  if (enabled) {
    parser->lenient_flags |= LENIENT_SPACES_AFTER_CHUNK_SIZE;
  } else {
    parser->lenient_flags &= ~LENIENT_SPACES_AFTER_CHUNK_SIZE;
  }
}

/* Callbacks */


inline int io::prot::http::llhttp::llhttp__on_message_begin(llhttp__internal_t* s, const unsigned char* p, const unsigned  char* endp) {
  int err;
   CALLBACK_MAYBE(s, on_message_begin);
  return err;
}


inline int io::prot::http::llhttp::llhttp__on_url(llhttp__internal_t* s, const unsigned  char* p, const unsigned  char* endp) {
  int err;
   SPAN_CALLBACK_MAYBE(s, on_url, p, endp - p);
  return err;
}


inline int io::prot::http::llhttp::llhttp__on_url_complete(llhttp__internal_t* s, const unsigned  char* p, const unsigned  char* endp) {
  int err;
   CALLBACK_MAYBE(s, on_url_complete);
  return err;
}


inline int io::prot::http::llhttp::llhttp__on_status(llhttp__internal_t* s, const unsigned  char* p, const unsigned  char* endp) {
  int err;
   SPAN_CALLBACK_MAYBE(s, on_status, p, endp - p);
  return err;
}


inline int io::prot::http::llhttp::llhttp__on_status_complete(llhttp__internal_t* s, const unsigned  char* p, const unsigned  char* endp) {
  int err;
   CALLBACK_MAYBE(s, on_status_complete);
  return err;
}


inline int io::prot::http::llhttp::llhttp__on_method(llhttp__internal_t* s, const unsigned  char* p, const unsigned  char* endp) {
  int err;
   SPAN_CALLBACK_MAYBE(s, on_method, p, endp - p);
  return err;
}


inline int io::prot::http::llhttp::llhttp__on_method_complete(llhttp__internal_t* s, const unsigned char* p, const unsigned char* endp) {
  int err;
   CALLBACK_MAYBE(s, on_method_complete);
  return err;
}


inline int io::prot::http::llhttp::llhttp__on_version(llhttp__internal_t* s, const unsigned char* p, const unsigned char* endp) {
  int err;
   SPAN_CALLBACK_MAYBE(s, on_version, p, endp - p);
  return err;
}


inline int io::prot::http::llhttp::llhttp__on_version_complete(llhttp__internal_t* s, const unsigned char* p, const unsigned char* endp) {
  int err;
   CALLBACK_MAYBE(s, on_version_complete);
  return err;
}


inline int io::prot::http::llhttp::llhttp__on_header_field(llhttp__internal_t* s, const unsigned char* p, const unsigned char* endp) {
  int err;
   SPAN_CALLBACK_MAYBE(s, on_header_field, p, endp - p);
  return err;
}


inline int io::prot::http::llhttp::llhttp__on_header_field_complete(llhttp__internal_t* s, const unsigned char* p, const unsigned char* endp) {
  int err;
   CALLBACK_MAYBE(s, on_header_field_complete);
  return err;
}


inline int io::prot::http::llhttp::llhttp__on_header_value(llhttp__internal_t* s, const unsigned char* p, const unsigned char* endp) {
  int err;
   SPAN_CALLBACK_MAYBE(s, on_header_value, p, endp - p);
  return err;
}


inline int io::prot::http::llhttp::llhttp__on_header_value_complete(llhttp__internal_t* s, const unsigned char* p, const unsigned char* endp) {
  int err;
   CALLBACK_MAYBE(s, on_header_value_complete);
  return err;
}


inline int io::prot::http::llhttp::llhttp__on_headers_complete(llhttp__internal_t* s, const unsigned char* p, const unsigned char* endp) {
  int err;
   CALLBACK_MAYBE(s, on_headers_complete);
  return err;
}


inline int io::prot::http::llhttp::llhttp__on_message_complete(llhttp__internal_t* s, const unsigned char* p, const unsigned char* endp) {
  int err;
   CALLBACK_MAYBE(s, on_message_complete);
  return err;
}


inline int io::prot::http::llhttp::llhttp__on_body(llhttp__internal_t* s, const unsigned char* p, const unsigned char* endp) {
  int err;
   SPAN_CALLBACK_MAYBE(s, on_body, p, endp - p);
  return err;
}


inline int io::prot::http::llhttp::llhttp__on_chunk_header(llhttp__internal_t* s, const unsigned char* p, const unsigned char* endp) {
  int err;
   CALLBACK_MAYBE(s, on_chunk_header);
  return err;
}


inline int io::prot::http::llhttp::llhttp__on_chunk_extension_name(llhttp__internal_t* s, const unsigned char* p, const unsigned char* endp) {
  int err;
   SPAN_CALLBACK_MAYBE(s, on_chunk_extension_name, p, endp - p);
  return err;
}


inline int io::prot::http::llhttp::llhttp__on_chunk_extension_name_complete(llhttp__internal_t* s, const unsigned char* p, const unsigned char* endp) {
  int err;
   CALLBACK_MAYBE(s, on_chunk_extension_name_complete);
  return err;
}


inline int io::prot::http::llhttp::llhttp__on_chunk_extension_value(llhttp__internal_t* s, const unsigned char* p, const unsigned char* endp) {
  int err;
   SPAN_CALLBACK_MAYBE(s, on_chunk_extension_value, p, endp - p);
  return err;
}


inline int io::prot::http::llhttp::llhttp__on_chunk_extension_value_complete(llhttp__internal_t* s, const unsigned char* p, const unsigned char* endp) {
  int err;
   CALLBACK_MAYBE(s, on_chunk_extension_value_complete);
  return err;
}


inline int io::prot::http::llhttp::llhttp__on_chunk_complete(llhttp__internal_t* s, const unsigned char* p, const unsigned char* endp) {
  int err;
   CALLBACK_MAYBE(s, on_chunk_complete);
  return err;
}


inline int io::prot::http::llhttp::llhttp__on_reset(llhttp__internal_t* s, const unsigned char* p, const unsigned char* endp) {
  int err;
   CALLBACK_MAYBE(s, on_reset);
  return err;
}

inline int io::prot::http::llhttp::llhttp__before_headers_complete(llhttp__internal_t* parser, const unsigned char* p,
    const unsigned char* endp) {
    /* Set this here so that on_headers_complete() callbacks can see it */
    if ((parser->flags & F_UPGRADE) &&
        (parser->flags & F_CONNECTION_UPGRADE)) {
        /* For responses, "Upgrade: foo" and "Connection: upgrade" are
         * mandatory only when it is a 101 Switching Protocols response,
         * otherwise it is purely informational, to announce support.
         */
        parser->upgrade =
            (parser->type == HTTP_REQUEST || parser->status_code == 101);
    }
    else {
        parser->upgrade = (parser->method == HTTP_CONNECT);
    }
    return 0;
}


/* Return values:
 * 0 - No body, `restart`, message_complete
 * 1 - CONNECT request, `restart`, message_complete, and pause
 * 2 - chunk_size_start
 * 3 - body_identity
 * 4 - body_identity_eof
 * 5 - invalid transfer-encoding for request
 */
inline int io::prot::http::llhttp::llhttp__after_headers_complete(llhttp__internal_t* parser, const unsigned char* p,
    const unsigned char* endp) {
    int hasBody;

    hasBody = parser->flags & F_CHUNKED || parser->content_length > 0;
    if (
        (parser->upgrade && (parser->method == HTTP_CONNECT ||
            (parser->flags & F_SKIPBODY) || !hasBody)) ||
        /* See RFC 2616 section 4.4 - 1xx e.g. Continue */
        (parser->type == HTTP_RESPONSE && parser->status_code == 101)
        ) {
        /* Exit, the rest of the message is in a different protocol. */
        return 1;
    }

    if (parser->type == HTTP_RESPONSE && parser->status_code == 100) {
        /* No body, restart as the message is complete */
        return 0;
    }

    /* See RFC 2616 section 4.4 */
    if (
        parser->flags & F_SKIPBODY ||         /* response to a HEAD request */
        (
            parser->type == HTTP_RESPONSE && (
                parser->status_code == 102 ||     /* Processing */
                parser->status_code == 103 ||     /* Early Hints */
                parser->status_code == 204 ||     /* No Content */
                parser->status_code == 304        /* Not Modified */
                )
            )
        ) {
        return 0;
    }
    else if (parser->flags & F_CHUNKED) {
        /* chunked encoding - ignore Content-Length header, prepare for a chunk */
        return 2;
    }
    else if (parser->flags & F_TRANSFER_ENCODING) {
        if (parser->type == HTTP_REQUEST &&
            (parser->lenient_flags & LENIENT_CHUNKED_LENGTH) == 0 &&
            (parser->lenient_flags & LENIENT_TRANSFER_ENCODING) == 0) {
            /* RFC 7230 3.3.3 */

            /* If a Transfer-Encoding header field
             * is present in a request and the chunked transfer coding is not
             * the final encoding, the message body length cannot be determined
             * reliably; the server MUST respond with the 400 (Bad Request)
             * status code and then close the connection.
             */
            return 5;
        }
        else {
            /* RFC 7230 3.3.3 */

            /* If a Transfer-Encoding header field is present in a response and
             * the chunked transfer coding is not the final encoding, the
             * message body length is determined by reading the connection until
             * it is closed by the server.
             */
            return 4;
        }
    }
    else {
        if (!(parser->flags & F_CONTENT_LENGTH)) {
            if (!llhttp_message_needs_eof(parser)) {
                /* Assume content-length 0 - read the next */
                return 0;
            }
            else {
                /* Read body until EOF */
                return 4;
            }
        }
        else if (parser->content_length == 0) {
            /* Content-Length header given but zero: Content-Length: 0\r\n */
            return 0;
        }
        else {
            /* Content-Length header given and non-zero */
            return 3;
        }
    }
}


inline int io::prot::http::llhttp::llhttp__after_message_complete(llhttp__internal_t* parser, const unsigned char* p,
    const unsigned char* endp) {
    int should_keep_alive;

    should_keep_alive = llhttp_should_keep_alive(parser);
    parser->finish = HTTP_FINISH_SAFE;
    parser->flags = 0;

    /* NOTE: this is ignored in loose parsing mode */
    return should_keep_alive;
}


inline int io::prot::http::llhttp::llhttp_message_needs_eof(const llhttp__internal_t* parser) {
    if (parser->type == HTTP_REQUEST) {
        return 0;
    }

    /* See RFC 2616 section 4.4 */
    if (parser->status_code / 100 == 1 || /* 1xx e.g. Continue */
        parser->status_code == 204 ||     /* No Content */
        parser->status_code == 304 ||     /* Not Modified */
        (parser->flags & F_SKIPBODY)) {     /* response to a HEAD request */
        return 0;
    }

    /* RFC 7230 3.3.3, see `llhttp__after_headers_complete` */
    if ((parser->flags & F_TRANSFER_ENCODING) &&
        (parser->flags & F_CHUNKED) == 0) {
        return 1;
    }

    if (parser->flags & (F_CHUNKED | F_CONTENT_LENGTH)) {
        return 0;
    }

    return 1;
}


inline int io::prot::http::llhttp::llhttp_should_keep_alive(const llhttp__internal_t* parser) {
    if (parser->http_major > 0 && parser->http_minor > 0) {
        /* HTTP/1.1 */
        if (parser->flags & F_CONNECTION_CLOSE) {
            return 0;
        }
    }
    else {
        /* HTTP/1.0 or earlier */
        if (!(parser->flags & F_CONNECTION_KEEP_ALIVE)) {
            return 0;
        }
    }

    return !llhttp_message_needs_eof(parser);
}



/* Private */