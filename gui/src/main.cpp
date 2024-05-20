#include "main_renderer.hpp"
#include <exception>
#include <filesystem>
#include <iostream>
#include <unistd.h>
#include <limits>

int main (void) 
{
    try {
        std::filesystem::create_directory("/var/dhcp/");
        std::filesystem::create_directory("/var/dhcp/database/");
        std::filesystem::create_directory("/var/log/dhcps/");
        std::filesystem::create_directory("/etc/dhcp/");
    } catch (std::exception &e) {
        std::cerr << e.what() << std::endl;
        std::cerr << "Please run the application with elevated priviledges" << std::endl;
        return 1;
    }

    auto myprivs = geteuid();
    if (myprivs != 0) {
        std::cerr << "You are running the application without root access." << std::endl 
            << "Application will run, but it's functionality will be limited." << std::endl 
            << "Consider reruning with root priviledges" << std::endl 
            << "Press Enter to start the application" << std::endl;

        std::cin.ignore(std::numeric_limits<std::streamsize>::max(),'\n');
    }

    return tui_loop();
}

