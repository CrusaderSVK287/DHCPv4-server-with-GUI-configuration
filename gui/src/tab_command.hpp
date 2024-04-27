
#ifndef __TAB_COMMAND_HPP__
#define __TAB_COMMAND_HPP__

#include "tab.hpp"
#include <cJSON.h>
#include <ftxui/component/component_base.hpp>
#include <string>
#include <vector>
class TabCommand;

struct Command {
    std::string name;
    bool ipc;
    void (TabCommand::*func)();
    std::string help;
    std::string usage;
};

class TabCommand : public UITab {
public:
    TabCommand();

    void refresh();
private:
    void initialize_commands();
    void print_response(cJSON *json);
    void handle_command();
    cJSON *send_command();

    int output_selected;
    std::vector<std::string> output;
    std::string input;
    std::vector<Command> commands;

    // Locally handled commands
    void command_clear();
    void command_help();
};

#endif //__TAB_COMMAND_HPP__
