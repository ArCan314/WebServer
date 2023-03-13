#include <unordered_map>
#include <string_view>

#include "./Logger.h"

static const std::unordered_map<std::string_view, std::string_view> kMimeTable =
    {
        {"aac", "audio/aac"},
        {"arc", "application/x-freearc"},
        {"avi", "video/x-msvideo"},
        {"bin", "application/octet-stream"},
        {"bmp", "image/bmp"},
        {"bz", "application/x-bzip"},
        {"bz2", "application/x-bzip2"},
        {"css", "text/css"},
        {"csv", "text/csv"},
        {"doc", "application/msword"},
        {"docx", "application/vnd.openxmlformats-officedocument.wordprocessingml.document"},
        {"eot", "application/vnd.ms-fontobject"},
        {"epub", "application/epub+zip"},
        {"gif", "image/gif"},
        {"htm", "text/html"},
        {"html", "text/html"},
        {"ico", "image/vnd.microsoft.icon"},
        {"jpeg", "image/jpeg"},
        {"jpg", "image/jpeg"},
        {"js", "text/javascript"},
        {"json", "application/json"},
        {"mjs", "text/javascript"},
        {"mp3", "audio/mpeg"},
        {"mp4", "video/mp4"},
        {"mpeg", "video/mpeg"},
        {"otf", "font/otf"},
        {"png", "image/png"},
        {"pdf", "application/pdf"},
        {"ppt", "application/vnd.ms-powerpoint"},
        {"rar", "application/x-rar-compressed"},
        {"svg", "image/svg+xml"},
        {"tar", "application/x-tar"},
        {"ttf","font/ttf"},
        {"txt","text/plain"},
        {"wav","audio/wav"},
        {"weba","audio/webm"},
        {"webm","video/webm"},
        {"webp","image/webp"},
        {"woff","font/woff"},
        {"woff2","font/woff2"},
        {"xml","text/xml"},
        {"zip","application/zip"},
};

static constexpr std::string_view kDefaultMime = "application/octet-stream";

std::string_view getMime(std::string_view ext_name)
{

    if (const auto iter = kMimeTable.find(ext_name); iter != kMimeTable.end())
        return iter->second;
    LOG_WARNING("Unknow Mime ext name = ", ext_name);
    return kDefaultMime;
}