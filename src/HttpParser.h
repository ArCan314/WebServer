#pragma once

#include <unordered_map>
#include <string_view>
#include <string>

#include <cctype>

#include "./HttpTypes.h"
#include "./Logger.h"

class HttpParser
{
private:
    HttpMethod method_{HttpMethod::NOT_SET};
    HttpVersion version_{HttpVersion::NOT_SET};
    std::string_view url_;
    std::string_view query_;
    std::string_view mime_;
    std::unordered_map<std::string_view, std::string_view> headers_;
    std::string raw_;

    static bool isNumber(std::string_view str)
    {
        return std::all_of(str.begin(), str.end(), [](int ch) { return std::isdigit(ch); });
    }

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
    bool isKeepAlive() const
    {
        if (const auto iter = headers_.find("Connection"); iter != headers_.end() && iter->second != "close")
            return true;
        return false;
    }
    long long getContentLength() const
    {
        try
        {
            if (const auto iter = headers_.find("Content-Length"); iter != headers_.end() && isNumber(iter->second))
                return std::atoll(iter->second.data());
            return 0;
        }
        catch (const std::exception &e)
        {
            LOG_WARNING("Exception raised, what = ", e.what());
            return 0;
        }
    }
    const auto &headers() const noexcept { return headers_; }
};