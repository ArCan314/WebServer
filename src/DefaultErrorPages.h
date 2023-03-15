#pragma once

#include <string>
#include <string_view>

#include "./HttpTypes.h"

std::string_view getDefaultErrorPage(const HttpStatusCode status_code);
std::string getErrorPageWithExtraMsg(const HttpStatusCode status_code, const std::string msg);