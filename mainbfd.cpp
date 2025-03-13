#include "bfd-dbus-server.hpp"
#include "logger.h"
#include <thread>
#include <signal.h>

static std::unique_ptr<sdbus::IConnection> connection;

static void signal_handler(int sig)
{
    connection->leaveEventLoop();
}

int main(){

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGABRT, signal_handler);

    std::shared_ptr<logger::Logger> logger;
    const std::string directoryPath = "/var/log/bfd";
    const std::string fileName = "bfd.log";
    logger = std::make_shared<logger::Logger>(directoryPath, fileName);
    logger->setLevel(logger::Level::Info);
    logger->info("main::bfd service was started");

    try
    {
        sdbus::ServiceName serviceName{"net.msystems.bfd"};
        connection = sdbus::createBusConnection(serviceName);

    
        sdbus::ObjectPath objectPath{"/net/msystems/bfd"};
        BfdDbusServer bfddbusserver(*connection, std::move(objectPath), logger);

        connection->enterEventLoop();
    }
    catch (const std::exception& e)
    {
        if (logger)
        {
            logger->error(e.what());
        }
        else
        {
            std::cerr << "Error: " << e.what() << '\n';
        }
    }
    catch (...)
    {
        if (logger)
        {
            logger->error("Unknown error\n");
        }
        else
        {
            std::cerr << "Unknown error\n";
        }
    }

    //

    return 0;
}