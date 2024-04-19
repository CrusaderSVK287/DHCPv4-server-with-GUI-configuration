#include "logger.hpp"

#ifdef DEBUG
#include <iostream>
#include <fstream>
#endif 

void log(std::string msg) {
#ifdef DEBUG
    std::ofstream logFile("_tui.log", std::ios::app); 
    if (logFile.is_open()) {
        logFile << msg << std::endl; 
        logFile.close(); 
    }
#endif
}

void log(const char *msg)
{
    std::string s = msg;
    log(s);
}

void log(char msg[])
{
    std::string s = msg;
    log(s);
}
