#include "main_renderer.hpp"
#include <exception>
#include <filesystem>
#include <iostream>

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

    return tui_loop();
}

