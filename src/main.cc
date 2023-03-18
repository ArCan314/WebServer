#include <iostream>

#include <csignal>

#include <unistd.h>

#include "./WebServer.h"

int main()
{
    std::cout << get_current_dir_name() << std::endl;
    std::signal(SIGPIPE, SIG_IGN);

    WebServer server{};
    server.addListenAddress("0.0.0.0", 12345, 3)
        .setLogPath("./server.log")
        .setLogLevel(LogLevel::INFO)
        .setRootPath("./root")
        .setWorkerThreadNum(3)
        .setWorkerPoolSize(4)
        .setAcceptorUsingEpoll(false);
        
    std::cout << "server thread total = " << server.getTotalThreadNum() << std::endl;
    server.start();

    return 0;
}