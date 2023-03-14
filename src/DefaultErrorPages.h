#pragma once

#include <string_view>

#include "./HttpTypes.h"

std::string_view getDefaultErrorPage(const HttpStatusCode status_code);