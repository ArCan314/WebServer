#include <fstream>
#include <chrono>
#include <thread>

#include <cstring>
#include <ctime>

#include <unistd.h>
#include <sys/types.h>

#include "./Logger.h"
#include "./util/utils.h"

static constexpr const char *kLevelPromptStr[] = 
{
    "[DEBUG  ]",
    "[INFO   ]",
    "[WARNING]",
    "[ERROR  ]",                    
};

static constexpr const char *kLogLevelStr[] = {
    "LogLevel::DEBUG",
    "LogLevel::INFO",
    "LogLevel::WARNING",
    "LogLevel::ERROR",
};

static std::string generateLogPath();
static std::string getTimeStr(std::string_view format);
static std::string getTimeStr();

static std::string generateLogPath()
{
    std::string log_path = "./";
    log_path.append(getTimeStr("%Y-%M-%d_%H:%M:%S"))
            .append(".log", lengthOfNullEndStr(".log"));
    return log_path;
}

static std::string getTimeStr()
{
    return getTimeStr("%Y/%m/%d-%H:%M:%S");
}

static std::string getTimeStr(std::string_view format)
{
    auto cur_time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    tm now;
    explicit_bzero(&now, sizeof(now));
    localtime_r(&cur_time, &now);
    
    static constexpr std::size_t kBufferSize = 100;
    std::array<char, kBufferSize> buffer;

    const int len = strftime(buffer.data(), sizeof(buffer), format.data(), &now);
    std::string res(std::begin(buffer), std::begin(buffer) + len);
    return res;
}

std::string_view getLogLevelStr(LogLevel level)
{
    static_assert(std::size(kLogLevelStr) == static_cast<int>(LogLevel::ERROR) + 1);
    return kLogLevelStr[static_cast<int>(level)];
}

bool Logger::start()
{
    if (worker_)
        return false;

    if (log_path_.empty())
        log_path_ = generateLogPath();

    log_stream_.open(log_path_, std::ios::out);
    if (!log_stream_)
        return false;

    worker_ = std::make_unique<std::thread>([this](){ this->work(); });
    return true;
}

void Logger::work()
{
    while (true)
    {
        auto msg = queue_.dequeue();
        if (!msg.has_value())
            return;

        log_stream_ << msg.value();
        log_stream_.flush();
    }
}

void Logger::log(const std::string &msg, LogLevel level, const char *file, int line)
{
    log(msg, level, file, line, nullptr);
}

void Logger::log(const std::string &msg, LogLevel level, const char *file, int line, const char *func)
{
    // [level][tid][time]:(file:line#func) msg
    std::string log_str(kLevelPromptStr[static_cast<int>(level)]);
    
    log_str.push_back('[');
    log_str.append(std::to_string(gettid()));
    log_str.push_back(']');

    log_str.push_back('[');
    log_str.append(getTimeStr());
    log_str.push_back(']');

    log_str.push_back(':');

    log_str.push_back('(');
    log_str.append(file);
    log_str.push_back(':');
    log_str.append(std::to_string(line));
    if (func)
    {
        log_str.push_back('#');
        log_str.append(func);
    }
    log_str.push_back(')');
    log_str.push_back(' ');
    
    log_str.append(msg);
    log_str.push_back('\n');
    queue_.enqueue(std::move(log_str));
}