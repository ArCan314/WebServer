#include <unordered_map>
#include <string_view>
#include <string>
#include <array>
#include <algorithm>

#include <cctype>
#include <cstring>

#include "./HttpTypes.h"
#include "./HttpParser.h"
#include "./util/utils.h"
#include "./Mime.h"

static bool parseMethod(std::string_view req, int &pos, HttpMethod &method);
static bool parseUrl(std::string_view req, int &out_pos, std::string_view &url_out, std::string_view &query, std::string_view &mime);
static bool parseVersion(std::string_view req, int &pos, HttpVersion &version);
static bool parseHeaders(std::string_view req, int &pos, std::unordered_map<std::string_view, std::string_view> &headers);
static bool ignoreOneSpace(std::string_view req, int &pos);
static bool isCRLF(std::string_view req, int pos);
static bool ignoreCRLF(std::string_view req, int &pos);

void HttpParser::clear()
{
    method_ = HttpMethod::NOT_SET;
    version_ = HttpVersion::NOT_SET;
    url_ = "";
    headers_.clear();
    raw_.clear();
}

int HttpParser::parse(std::string request)
{
    clear();
    headers_.reserve(10);

    raw_ = std::move(request);
    int pos = 0;

    // request-line   = method SP request-target SP HTTP-version CRLF # rfc7230 sec:3.1
    if (!parseMethod(raw_, pos, method_))
        return false;

    if (!ignoreOneSpace(raw_, pos))
        return false;

    if (!parseUrl(raw_, pos, url_, query_, mime_))
        return false;

    if (!ignoreOneSpace(raw_, pos))
        return false;

    if (!parseVersion(raw_, pos, version_))
        return false;

    if (!ignoreCRLF(raw_, pos))
        return false;

    // headers
    if (!parseHeaders(raw_, pos, headers_))
        return false;

    // end of http request header
    if (!ignoreCRLF(raw_, pos))
        return false;

    head_length_ = pos;
    return pos;
}

bool parseMethod(std::string_view req, int &pos, HttpMethod &method)
{
    static constexpr const char *kMethodStr[] = {
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

    static constexpr HttpMethod kMethodValue[] = {
        HttpMethod::GET,
        HttpMethod::HEAD,
        HttpMethod::POST,
        HttpMethod::PUT,
        HttpMethod::DELETE,
        HttpMethod::TRACE,
        HttpMethod::OPTIONS,
        HttpMethod::CONNECT,
        HttpMethod::PATCH,
    };
    static_assert(std::size(kMethodStr) == std::size(kMethodValue));

    for (int i = 0; i < std::size(kMethodStr); i++)
        if (req.substr(pos, std::strlen(kMethodStr[i])) == kMethodStr[i])
        {
            method = kMethodValue[i];
            pos += std::strlen(kMethodStr[i]);
            return true;
        }

    return false;
}

bool parseUrl(std::string_view req, int &out_pos, std::string_view &url_out, std::string_view &query, std::string_view &mime)
{
    // origin-form    = absolute-path [ "?" query ]
    // query       = *( pchar / "/" / "?" ) The query component is indicated 
    // by the first question mark ("?") character and terminated by a number 
    // sign ("#") character or by the end of the URI. RFC3986 sec#3.4

    const auto space_pos = req.find(' ', out_pos);
    if (space_pos == std::string_view::npos)
        return false;

    const auto question_mark_pos = std::find(req.begin() + out_pos, req.begin() + space_pos, '?');
    if (question_mark_pos != req.begin() + space_pos)
        query = req.substr(std::distance(req.begin(), std::next(question_mark_pos)), std::distance(std::next(question_mark_pos), req.begin() + space_pos));

    url_out = req.substr(out_pos, std::distance(req.begin() + out_pos, question_mark_pos));

    int dot_pos = url_out.size() - 1;
    while (dot_pos >= 0 && url_out[dot_pos] != '/' && url_out[dot_pos]  != '.')
        dot_pos--;

    if (dot_pos < 0 || url_out[dot_pos] == '/')
        mime = "text/html";
    else
        mime = getMime(url_out.substr(dot_pos + 1));

    out_pos = space_pos;
    return true;
}

bool parseVersion(std::string_view req, int &pos, HttpVersion &version)
{
    if (pos + lengthOfNullEndStr("HTTP/X.X") >= req.size())
        return false;

    if (req.substr(pos, lengthOfNullEndStr("HTTP/")) != "HTTP/")
        return false;

    if (auto version_sv = req.substr(pos + lengthOfNullEndStr("HTTP/"), 3); version_sv == "1.0")
        version = HttpVersion::HTTP10;
    else if (version_sv == "1.1")
        version = HttpVersion::HTTP11;
    else if (version_sv == "2.0")
        version = HttpVersion::HTTP20;
    else if (version_sv == "3.0")
        version = HttpVersion::HTTP30;

    pos += lengthOfNullEndStr("HTTP/X.X");
    return true;
}

static bool parseHeaders(std::string_view req, int &pos, 
                         std::unordered_map<std::string_view, std::string_view> &headers)
{
    // rfc7230 sec:3.2
    // header-field   = field-name ":" OWS field-value OWS
    // OWS: optional whitespace
    // OWS            = *( SP / HTAB )
    // field-name     = token
    // field-value    = *( field-content / obs-fold )
    // field-content  = field-vchar [ 1*( SP / HTAB ) field-vchar ]
    // field-vchar    = VCHAR / obs-text
    // obs-fold       = CRLF 1*( SP / HTAB )

    auto hint = headers.begin();
    while (!isCRLF(req, pos))
    {
        const int next_crlf_pos = req.find("\r\n", pos);
        if (next_crlf_pos == std::string_view::npos)
            return false;

        const int colon_pos = req.find(':', pos);
        if (colon_pos == std::string_view::npos)
            return false;
        
        // TODO: validate field-name

        // no whitespace between field-name and colon
        for (int i = pos; i < colon_pos; i++)
            if (std::isspace(req[pos]))
                return false;

        int value_start_pos = colon_pos + 1;
        while (value_start_pos < req.size() && std::isspace(req[value_start_pos]))
            value_start_pos++;
        
        int value_end_pos = next_crlf_pos - 1;
        while (value_end_pos >= value_start_pos && std::isspace(req[value_end_pos]))
            value_end_pos--;
        
        hint = headers.emplace_hint(hint, req.substr(pos, colon_pos - pos), req.substr(value_start_pos, value_end_pos - value_start_pos + 1));
        pos = next_crlf_pos + 2;
    }
    return true;
}

bool ignoreOneSpace(std::string_view req, int &pos)
{
    if (pos < req.size() && req[pos] == ' ')
    {
        pos++;
        return true;
    }
    return false;
}

bool isCRLF(std::string_view req, int pos)
{
    return (pos + 1 < req.size() && req[pos] == '\r' && req[pos + 1] == '\n');        
}

bool ignoreCRLF(std::string_view req, int &pos)
{
    if (isCRLF(req, pos))
    {
        pos += 2;
        return true;
    }
    return false;
}