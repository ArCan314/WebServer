#pragma once

enum class HttpMethod
{
    NOT_SET = 0,
    GET,
    HEAD,
    POST,
    PUT,
    DELETE,
    TRACE,
    OPTIONS,
    CONNECT,
    PATCH,
};

inline constexpr const char *kHttpMethodStr[] = {
    "ERROR",
    "GET",
    "HEAD",
    "POST",
    "PUT",
    "DELETE",
    "TRACE",
    "OPTIONS",
    "CONNECT",
    "PATCH",
};

enum class HttpVersion: unsigned
{
    NOT_SET = 0,
    HTTP10 = 10,
    HTTP11 = 11,
    HTTP20 = 20,
    HTTP30 = 30,
};

enum class HttpStatusCode: unsigned
{
    CONTINUE = 100,
    OK = 200,
    MOVED_PERMANENTLY = 301,
    FOUND = 302,
    NOT_MODIFIED = 304,
    TEMPORARY_REDIRECT = 307,
    BAD_REQUEST = 400,
    UNAUTHORIZED = 401,
    FORBIDDEN = 403,
    NOT_FOUND = 404,
    PROXY_AUTH_REQUIRED = 407,
    INTERNAL_SERVER_ERROR = 500,
    NOT_IMPLEMENTED = 501,
    SERVICE_UNAVAILABLE = 503,
    HTTP_VERSION_NOT_SUPPORTED = 505,
};


