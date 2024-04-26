#include "tab_command.hpp"

void TabCommand::command_clear()
{
    output.clear();
    output_selected = 0;
}

void TabCommand::command_help()
{
    output.push_back("Short description of each command");

    for(auto &entry : commands) {
        output.push_back("  " + entry.name + " : " + entry.help);
    }
}

