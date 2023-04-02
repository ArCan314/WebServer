#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <thread>
#include <fstream>
#include <sstream>
#include <iostream>
#include <type_traits>
#include <array>

#include <string.h>

#include "./util/Queue.h"
#include "./util/Singleton.h"

enum class LogLevel : int
{
    DEBUG = 0,
    INFO = 1,
    WARNING = 2,
    ERROR = 3,
};

std::string_view getLogLevelStr(LogLevel level);

class Logger : public Singleton<Logger>
{
private:
    Queue<std::string> queue_;
    LogLevel level_{LogLevel::WARNING};
    std::unique_ptr<std::thread> worker_;
    std::string log_path_;
    std::ofstream log_stream_;

    void work();

public:
    bool start();
    Logger &setLevel(LogLevel level) noexcept
    {
        level_ = level;
        return *this;
    }
    Logger &setPath(std::string path) noexcept
    {
        log_path_ = std::move(path);
        return *this;
    }

    void log(const std::string &msg, LogLevel level, const char *file, int line);
    void log(const std::string &msg, LogLevel level, const char *file, int line, const char *func);
    LogLevel getLevel() const noexcept { return level_; }
};

// template <typename... Args>
// inline std::string logstr(Args ...args)
// {
//     thread_local std::stringstream ss;

//     ss.str(std::string());
//     ((ss << args), ...);
//     return ss.str();
// }

template <typename T>
inline std::size_t __logstrsize(const T &arg)
{

    if constexpr (std::is_arithmetic_v<std::decay_t<T>>)
    {
        static constexpr std::size_t kPossibleNumberLength = 10;
        return kPossibleNumberLength;
    }
    else if constexpr (std::is_same_v<T, const char *>)
        return strlen(arg);
    else
        return std::size(arg);
}

template <typename T, typename... Args>
inline std::size_t __logstrsize(const T &arg, const Args &...args)
{
    if constexpr (std::is_arithmetic_v<std::decay_t<T>>)
    {
        static constexpr std::size_t kPossibleNumberLength = 10;
        return kPossibleNumberLength + __logstrsize(args...);
    }
    else if constexpr (std::is_same_v<T, const char *>)
        return strlen(arg) + __logstrsize(args...);
    else
        return std::size(arg) + __logstrsize(args...);
}

template <typename T>
inline void __logstr(std::string &s, const T &arg)
{
    if constexpr (std::is_arithmetic_v<std::decay_t<T>>)
        s.append(std::to_string(arg));
    else
        s.append(arg);
}

template <typename T, typename... Args>
inline void __logstr(std::string &s, const T &arg, const Args &...args)
{
    if constexpr (std::is_arithmetic_v<std::decay_t<T>>)
        s.append(std::to_string(arg));
    else
        s.append(arg);
    __logstr(s, args...);
}

template <typename... Args>
inline std::string logstr(const Args &...args)
{
    std::string res;
    res.reserve(__logstrsize(args...));
    __logstr(res, args...);
    return res;
}

inline std::string logErrStr(int err_no)
{
    static constexpr std::size_t kBufferSize = 100;
    std::array<char, kBufferSize> buffer;
    const char *retval = strerror_r(err_no, buffer.begin(), std::size(buffer));
    return std::string{retval};
}

// #define NO_LOG
#ifdef NO_LOG

#define LOG_HELPER(msg, level) \
    do                         \
    {                          \
    } while (0)

#else

#define LOG_HELPER(msg, level)                                                          \
    do                                                                                  \
    {                                                                                   \
        if (static_cast<int>(level) >= static_cast<int>(Logger::instance().getLevel())) \
            Logger::instance().log(msg, level, __FILE__, __LINE__, __func__);           \
    } while (0)

#endif

#define LOG_DEBUG(msg, ...) LOG_HELPER(logstr(msg, ##__VA_ARGS__), LogLevel::DEBUG)
#define LOG_INFO(msg, ...) LOG_HELPER(logstr(msg, ##__VA_ARGS__), LogLevel::INFO)
#define LOG_WARNING(msg, ...) LOG_HELPER(logstr(msg, ##__VA_ARGS__), LogLevel::WARNING)
#define LOG_ERROR(msg, ...) LOG_HELPER(logstr(msg, ##__VA_ARGS__), LogLevel::ERROR)

#define ___LOGIF(x, falthy, level, msg, ...) \
    do                                       \
    {                                        \
        if (x == falthy)                     \
            LOG_##level(msg, ##__VA_ARGS__); \
    } while (0)

// LOG macros that check return value
#define LOGIF_BDEBUG(x, msg, ...) ___LOGIF(x, false, DEBUG, msg, ##__VA_ARGS__)
#define LOGIF_BINFO(x, msg, ...) ___LOGIF(x, false, INFO, msg, ##__VA_ARGS__)
#define LOGIF_BWARNING(x, msg, ...) ___LOGIF(x, false, WARNING, msg, ##__VA_ARGS__)
#define LOGIF_BERROR(x, msg, ...) ___LOGIF(x, false, ERROR, msg, ##__VA_ARGS__)
#define LOGIF_PDEBUG(x, msg, ...) ___LOGIF(x, -1, DEBUG, msg, ##__VA_ARGS__)
#define LOGIF_PINFO(x, msg, ...) ___LOGIF(x, -1, INFO, msg, ##__VA_ARGS__)
#define LOGIF_PWARNING(x, msg, ...) ___LOGIF(x, -1, WARNING, msg, ##__VA_ARGS__)
#define LOGIF_PERROR(x, msg, ...) ___LOGIF(x, -1, ERROR, msg, ##__VA_ARGS__)

#define LOG_STDERR(msg, ...)                                \
    do                                                      \
    {                                                       \
        std::cerr << logstr(msg, __VA_ARGS__) << std::endl; \
    } while (0)
