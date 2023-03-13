#include <iostream>

#include <csignal>

#include <unistd.h>

#include "./WebServer.h"

int main()
{
    std::cout << get_current_dir_name() << std::endl;
    std::signal(SIGPIPE, SIG_IGN);
    WebServer server("./root", LogLevel::WARNING);

    static constexpr int kPort = 12345;
    static constexpr int kEventLoopThreadCount = 3;
    server.start("0.0.0.0", kPort, 3, 3, 4);

    return 0;
}