#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "./HttpTypes.h"

class HttpResponseBuilder
{
private:
    HttpVersion version_{HttpVersion::HTTP11};
    HttpStatusCode status_code_{HttpStatusCode::OK};
    std::string reason_{};
    std::string body_{};
    std::vector<std::string> headers_;
    int headers_len_{0};
public:
    HttpResponseBuilder(HttpStatusCode status = HttpStatusCode::OK, HttpVersion version = HttpVersion::HTTP11) noexcept;
    HttpResponseBuilder &setVersion(HttpVersion version) noexcept;
    HttpResponseBuilder &setStatusCode(HttpStatusCode status) noexcept;
    HttpResponseBuilder &setDefaultErrorPage(HttpStatusCode status_code) noexcept;
    HttpResponseBuilder &setReason(std::string reason) noexcept;
    HttpResponseBuilder &addHeader(std::string name, std::string value);
    HttpResponseBuilder &setBody(std::string body);
    auto bodySize() const noexcept { return body_.size(); }
    std::string build();
    std::string buildOnce();
    std::string buildNoBody();
    std::string buildNoBodyOnce();
    void clear();
};