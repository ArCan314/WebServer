#include <algorithm>
#include <unordered_map>

#include <cassert>

#include "./HttpTypes.h"
#include "./HttpResponseBuilder.h"
#include "./DefaultErrorPages.h"
#include "./Logger.h"

static const std::unordered_map<HttpStatusCode, const char *> kDefaultReason =
    {
        {HttpStatusCode::CONTINUE, "Continue"},
        {HttpStatusCode::OK, "Ok"},
        {HttpStatusCode::MOVED_PERMANENTLY, "Moved Permanently"},
        {HttpStatusCode::FOUND, "Found"},
        {HttpStatusCode::NOT_MODIFIED, "Not Modified"},
        {HttpStatusCode::TEMPORARY_REDIRECT, "Temporary Rediredt"},
        {HttpStatusCode::BAD_REQUEST, "Bad Request"},
        {HttpStatusCode::UNAUTHORIZED, "Unauthorized"},
        {HttpStatusCode::FORBIDDEN, "Forbidden"},
        {HttpStatusCode::NOT_FOUND, "Not Found"},
        {HttpStatusCode::PROXY_AUTH_REQUIRED, "Proxy Authentication Required"},
        {HttpStatusCode::INTERNAL_SERVER_ERROR, "Internal Server Error"},
        {HttpStatusCode::NOT_IMPLEMENTED, "Not Implemented"},
        {HttpStatusCode::SERVICE_UNAVAILABLE, "Service Unavailable"},
        {HttpStatusCode::HTTP_VERSION_NOT_SUPPORTED, "HTTP Version Not Supported"},
};

// std::unordered_map<std::string, std::string> HttpResponseBuilder::default_headers_{};

HttpResponseBuilder::HttpResponseBuilder(HttpStatusCode status_code, HttpVersion version) noexcept
    : version_(version),
      status_code_(status_code),
      body_((static_cast<uint32_t>(status_code) >= 400u) ? getDefaultErrorPage(status_code) : "")
{
}

HttpResponseBuilder &HttpResponseBuilder::setVersion(HttpVersion version) noexcept
{
    version_ = version;
    return *this;
}

HttpResponseBuilder &HttpResponseBuilder::setStatusCode(HttpStatusCode status_code) noexcept
{
    status_code_ = status_code;
    return *this;
}

HttpResponseBuilder &HttpResponseBuilder::setDefaultErrorPage(HttpStatusCode status_code) noexcept
{
    if (static_cast<unsigned>(status_code) >= 400u)
        body_ = getDefaultErrorPage(status_code);
    return *this;
}

HttpResponseBuilder &HttpResponseBuilder::setReason(std::string reason) noexcept
{
    reason_ = std::move(reason);
    return *this;
}

HttpResponseBuilder &HttpResponseBuilder::addHeader(std::string name, std::string value)
{
    headers_len_ += name.size() + value.size();
    headers_.emplace_back(std::move(name));
    headers_.emplace_back(std::move(value));
    return *this;
}

HttpResponseBuilder &HttpResponseBuilder::setBody(std::string body)
{
    body_.swap(body);
    return *this;
}

static inline void pushStatusCode(std::string &str, HttpStatusCode code)
{
    const int prev_size = str.size();
    int num = static_cast<int>(code);
    while (num)
    {
        str.push_back('0' + num % 10);
        num /= 10;
    }

    std::reverse(str.begin() + prev_size, str.end());
}

std::string HttpResponseBuilder::build()
{
    auto res = buildNoBody();
    res.append(body_);
    return res;
}

std::string HttpResponseBuilder::buildOnce()
{
    std::string res = build();
    clear();
    return res;
}

std::string HttpResponseBuilder::buildNoBody()
{
    // status-line = HTTP-version SP status-code SP reason-phrase CRLF #rfc7230 sec:3.1.2
    std::string res("HTTP/");
    res.reserve(headers_len_ + 25);

    res.push_back('0' + static_cast<int>(version_) / 10);
    res.push_back('.');
    res.push_back('0' + static_cast<int>(version_) % 10);

    res.push_back(' ');

    pushStatusCode(res, status_code_);
    res.push_back(' ');

    res.append(reason_.empty() ? kDefaultReason.at(status_code_) : reason_);
    res.append("\r\n", 2);

    for (int i = 0; i < headers_.size(); i += 2)
    {
        res.append(headers_[i])
            .append(":", 1)
            .append(headers_[i + 1])
            .append("\r\n", 2);
    }

    res.append("\r\n", 2);
    return res;
}

std::string HttpResponseBuilder::buildNoBodyOnce()
{
    std::string res = buildNoBody();
    clear();
    return res;
}

void HttpResponseBuilder::clear()
{
    version_ = HttpVersion::HTTP11;
    status_code_ = HttpStatusCode::OK;
    reason_.clear();
    body_.clear();
    headers_.clear();
    headers_len_ = 0;
}