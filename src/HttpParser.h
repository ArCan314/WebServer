#pragma once

#include <unordered_map>
#include <string_view>
#include <string>

#include <cctype>

#include "./HttpTypes.h"

class HttpParser
{
public:
    HttpParser() = default;
    int parse(std::string request);
    void clear();
    auto method() const noexcept { return method_; }
    auto version() const noexcept { return version_; }
    auto url() const noexcept { return url_; }
    auto mime() const noexcept { return mime_; }
    auto query() const noexcept { return query_; }
    bool hasQuery() const noexcept { return query_.size() > 0; }
    const auto &headers() const noexcept { return headers_; }
    auto headLength() const noexcept { return head_length_; }

    bool isKeepAlive() const;
    long long getContentLength() const;

private:
    HttpMethod method_{HttpMethod::NOT_SET};
    HttpVersion version_{HttpVersion::NOT_SET};
    std::string_view url_;
    std::string_view query_;
    std::string_view mime_;
    std::unordered_map<std::string_view, std::string_view> headers_;
    std::string raw_;
    int head_length_{};
};