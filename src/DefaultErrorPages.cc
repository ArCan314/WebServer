#include <string_view>

#include "./HttpTypes.h"
#include "./DefaultErrorPages.h"

#define EHTML(x, ...) x __VA_ARGS__
#define TITLE(x) "<head><title>" x "</title></head>"
#define ERROR_MSG(x) "<body><h1>" x "</h1></body>"
#define HTML(x, ...) "<html>" EHTML(x, __VA_ARGS__) "</html>"

static constexpr const char kBadRequest[] = HTML(TITLE("400 Bad Request"), ERROR_MSG("400 BadRequest"));
static constexpr const char kUnauthorized[] = HTML(TITLE("401 Unauthorized"), ERROR_MSG("401 Unauthorized"));
static constexpr const char kForbidden[] = HTML(TITLE("403 Forbidden"), ERROR_MSG("403 Forbidden"));
static constexpr const char kNotFound[] = HTML(TITLE("404 Not Found"), ERROR_MSG("404 Not Found"));
static constexpr const char kProxyAuthRequired[] = HTML(TITLE("407 Proxy Authentication Required"), ERROR_MSG("407 Proxy Authentication Required"));
static constexpr const char kInternalServerError[] = HTML(TITLE("500 Internal Server Error"), ERROR_MSG("500 Internal Server Error"));
static constexpr const char kNotImplemented[] = HTML(TITLE("501 Not Implemented"), ERROR_MSG("501 Not Implemented"));
static constexpr const char kServiceUnavailable[] = HTML(TITLE("503 Service Unavailable"), ERROR_MSG("503 Service Unavailable"));
static constexpr const char kHttpVersionNotSupported[] = HTML(TITLE("505 HTTP Version Not Supported"), ERROR_MSG("505 HTTP Version Not Supported"));

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