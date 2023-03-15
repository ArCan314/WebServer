#include <string>
#include <string_view>

#include "./HttpTypes.h"
#include "./DefaultErrorPages.h"

#define HTML_BEGIN "<html>"
#define HTML_END "</html>"
#define HEAD_BEGIN "<head>"
#define HEAD_END "</head>"
#define TITLE_BEGIN "<title>"
#define TITLE_END "</title>"
#define BODY_BEGIN "<body>"
#define BODY_END "</body>"
#define H1_BEGIN "<h1>"
#define H1_END "</h1>"
#define P_BEGIN "<p>"
#define P_END "</p>"

#define EXPAND(x, ...) x __VA_ARGS__

#define _P(x, ...) P_BEGIN EXPAND(x, __VA_ARGS__) P_END
#define _HTML(x, ...) HTML_BEGIN EXPAND(x, __VA_ARGS__) HTML_END
#define _H1(x, ...) H1_BEGIN EXPAND(x, __VA_ARGS__) H1_END
#define _HEAD(x, ...) HEAD_BEGIN EXPAND(x, __VA_ARGS__) HEAD_END
#define _TITLE(x, ...) TITLE_BEGIN EXPAND(x, __VA_ARGS__) TITLE_END
#define _BODY(x, ...) BODY_BEGIN EXPAND(x, __VA_ARGS__) BODY_END

#define TITLE(x) _HEAD(_TITLE(x))
#define ERROR_MSG(x) _BODY(_H1(x))
#define HTML(x, ...) _HTML(x, __VA_ARGS__)

#define BAD_REQUEST_ERROR_MSG "400 Bad Request"
#define UNAUTHORIZED_ERROR_MSG "401 Unauthorized"
#define FORBIDDEN_ERROR_MSG "403 Forbidden"
#define NOT_FOUND_ERROR_MSG "404 Not Found"
#define PROXY_AUTH_REQUIRED_ERROR_MSG "407 Proxy Authentication Required"
#define INTERNAL_SERVER_ERROR_ERROR_MSG "500 Internal Server Error"
#define NOT_IMPLEMENTED_ERROR_MSG "501 Not Implemented"
#define SERVICE_UNAVAILABLE_ERROR_MSG "503 Service Unavailable"
#define HTTP_VERSION_NOT_SUPPORTED_ERROR_MSG "505 HTTP Version Not Supported"

#define BAD_REQUEST_TITLE TITLE(BAD_REQUEST_ERROR_MSG)
#define UNAUTHORIZED_TITLE TITLE(UNAUTHORIZED_ERROR_MSG)
#define FORBIDDEN_TITLE TITLE(FORBIDDEN_ERROR_MSG)
#define NOT_FOUND_TITLE TITLE(NOT_FOUND_ERROR_MSG)
#define PROXY_AUTH_REQUIRED_TITLE TITLE(PROXY_AUTH_REQUIRED_ERROR_MSG)
#define INTERNAL_SERVER_ERROR_TITLE TITLE(INTERNAL_SERVER_ERROR_ERROR_MSG)
#define NOT_IMPLEMENTED_TITLE TITLE(NOT_IMPLEMENTED_ERROR_MSG)
#define SERVICE_UNAVAILABLE_TITLE TITLE(SERVICE_UNAVAILABLE_ERROR_MSG)
#define HTTP_VERSION_NOT_SUPPORTED_TITLE TITLE(HTTP_VERSION_NOT_SUPPORTED_ERROR_MSG)

static constexpr const char kBadRequest[] = HTML(BAD_REQUEST_TITLE, ERROR_MSG(BAD_REQUEST_ERROR_MSG));
static constexpr const char kUnauthorized[] = HTML(UNAUTHORIZED_TITLE, ERROR_MSG(UNAUTHORIZED_ERROR_MSG));
static constexpr const char kForbidden[] = HTML(FORBIDDEN_TITLE, ERROR_MSG(FORBIDDEN_ERROR_MSG));
static constexpr const char kNotFound[] = HTML(NOT_FOUND_TITLE, ERROR_MSG(NOT_FOUND_ERROR_MSG));
static constexpr const char kProxyAuthRequired[] = HTML(PROXY_AUTH_REQUIRED_TITLE, ERROR_MSG(PROXY_AUTH_REQUIRED_ERROR_MSG));
static constexpr const char kInternalServerError[] = HTML(INTERNAL_SERVER_ERROR_TITLE, ERROR_MSG(INTERNAL_SERVER_ERROR_ERROR_MSG));
static constexpr const char kNotImplemented[] = HTML(NOT_IMPLEMENTED_TITLE, ERROR_MSG(NOT_IMPLEMENTED_ERROR_MSG));
static constexpr const char kServiceUnavailable[] = HTML(SERVICE_UNAVAILABLE_TITLE, ERROR_MSG(SERVICE_UNAVAILABLE_ERROR_MSG));
static constexpr const char kHttpVersionNotSupported[] = HTML(HTTP_VERSION_NOT_SUPPORTED_TITLE, ERROR_MSG(HTTP_VERSION_NOT_SUPPORTED_ERROR_MSG));

std::string_view getDefaultErrorPage(const HttpStatusCode status_code)
{
    switch (status_code)
    {
    case HttpStatusCode::BAD_REQUEST:
        return kBadRequest;
    case HttpStatusCode::UNAUTHORIZED:
        return kUnauthorized;
    case HttpStatusCode::FORBIDDEN:
        return kForbidden;
    case HttpStatusCode::NOT_FOUND:
        return kNotFound;
    case HttpStatusCode::PROXY_AUTH_REQUIRED:
        return kProxyAuthRequired;
    case HttpStatusCode::INTERNAL_SERVER_ERROR:
        return kInternalServerError;
    case HttpStatusCode::NOT_IMPLEMENTED:
        return kNotImplemented;
    case HttpStatusCode::SERVICE_UNAVAILABLE:
        return kServiceUnavailable;
    case HttpStatusCode::HTTP_VERSION_NOT_SUPPORTED:
        return kHttpVersionNotSupported;
    default:
        return "";
    }
}

std::string getErrorPageWithExtraMsg(const HttpStatusCode status_code, const std::string msg)
{
    std::string res;
    switch (status_code)
    {
    case HttpStatusCode::BAD_REQUEST:
        res.append(HTML_BEGIN
                        BAD_REQUEST_TITLE 
                        BODY_BEGIN
                            _H1(BAD_REQUEST_ERROR_MSG)
                            P_BEGIN
                            )
           .append(msg)
           .append(
                            P_END
                        BODY_END
                    HTML_END);
        break;
    case HttpStatusCode::UNAUTHORIZED:
        res.append(HTML_BEGIN
                        UNAUTHORIZED_TITLE 
                        BODY_BEGIN
                            _H1(UNAUTHORIZED_ERROR_MSG)
                            P_BEGIN
                            )
           .append(msg)
           .append(
                            P_END
                        BODY_END
                    HTML_END);
        break;
    case HttpStatusCode::FORBIDDEN:
        res.append(HTML_BEGIN
                        FORBIDDEN_TITLE 
                        BODY_BEGIN
                            _H1(FORBIDDEN_ERROR_MSG)
                            P_BEGIN
                            )
           .append(msg)
           .append(
                            P_END
                        BODY_END
                    HTML_END);
        break;
    case HttpStatusCode::NOT_FOUND:
        res.append(HTML_BEGIN
                        NOT_FOUND_TITLE 
                        BODY_BEGIN
                            _H1(NOT_FOUND_ERROR_MSG)
                            P_BEGIN
                            )
           .append(msg)
           .append(
                            P_END
                        BODY_END
                    HTML_END);
        break;
    case HttpStatusCode::PROXY_AUTH_REQUIRED:
        res.append(HTML_BEGIN
                        PROXY_AUTH_REQUIRED_TITLE 
                        BODY_BEGIN
                            _H1(PROXY_AUTH_REQUIRED_ERROR_MSG)
                            P_BEGIN
                            )
           .append(msg)
           .append(
                            P_END
                        BODY_END
                    HTML_END);
        break;
    case HttpStatusCode::INTERNAL_SERVER_ERROR:
        res.append(HTML_BEGIN
                        INTERNAL_SERVER_ERROR_TITLE 
                        BODY_BEGIN
                            _H1(INTERNAL_SERVER_ERROR_ERROR_MSG)
                            P_BEGIN
                            )
           .append(msg)
           .append(
                            P_END
                        BODY_END
                    HTML_END);
        break;
    case HttpStatusCode::NOT_IMPLEMENTED:
        res.append(HTML_BEGIN
                        NOT_IMPLEMENTED_TITLE 
                        BODY_BEGIN
                            _H1(NOT_IMPLEMENTED_ERROR_MSG)
                            P_BEGIN
                            )
           .append(msg)
           .append(
                            P_END
                        BODY_END
                    HTML_END);
        break;
    case HttpStatusCode::SERVICE_UNAVAILABLE:
        res.append(HTML_BEGIN
                        SERVICE_UNAVAILABLE_TITLE 
                        BODY_BEGIN
                            _H1(SERVICE_UNAVAILABLE_ERROR_MSG)
                            P_BEGIN
                            )
           .append(msg)
           .append(
                            P_END
                        BODY_END
                    HTML_END);
        break;
    case HttpStatusCode::HTTP_VERSION_NOT_SUPPORTED:
        res.append(HTML_BEGIN
                        HTTP_VERSION_NOT_SUPPORTED_TITLE 
                        BODY_BEGIN
                            _H1(HTTP_VERSION_NOT_SUPPORTED_ERROR_MSG)
                            P_BEGIN
                            )
           .append(msg)
           .append(
                            P_END
                        BODY_END
                    HTML_END);
        break;
    default:
        break;
    }
    return res;
}