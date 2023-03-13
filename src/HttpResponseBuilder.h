#pragma once

#include <string>
#include <string_view>
#include <unordered_map>

#include "./HttpTypes.h"
#include "./Logger.h"

class HttpResponseBuilder
{
private:
    HttpVersion version_{HttpVersion::HTTP11};
    HttpStatusCode status_code_{HttpStatusCode::OK};
    std::string reason_{};
    std::vector<std::string> headers_;
    int headers_len_{0};
public:
    HttpResponseBuilder(HttpStatusCode status = HttpStatusCode::OK, HttpVersion version = HttpVersion::HTTP11) noexcept;
    HttpResponseBuilder &setVersion(HttpVersion version) noexcept;
    HttpResponseBuilder &setStatusCode(HttpStatusCode status) noexcept;
    HttpResponseBuilder &setReason(std::string reason) noexcept;
    HttpResponseBuilder &addHeader(std::string name, std::string value);
    std::string build();
};