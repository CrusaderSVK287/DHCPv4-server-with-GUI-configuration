#include "tab_logs.hpp"
#include <filesystem>
#include <ftxui/component/component.hpp>
#include <ftxui/component/component_base.hpp>
#include <ftxui/dom/deprecated.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/dom/node.hpp>
#include <string>
#include <fstream>

using namespace ftxui;
namespace fs = std::filesystem;

TabLogs::TabLogs()
{
    this->selected = 0;
    this->log_selected = 0;
    this->load_entries();

    this->tab_contents = Container::Horizontal({
        Menu({&this->entries, &this->selected}) | vscroll_indicator | yframe | yflex,
        Menu({&this->log_content, &this->log_selected}) | vscroll_indicator | yframe | xflex_shrink,
    });
}

void TabLogs::load_entries()
{
    this->entries.clear();

    for (const auto &entry : fs::directory_iterator(TabLogs::logs_path)) {
        this->entries.push_back(entry.path().string().substr(24, 19));
    }
}

void TabLogs::load_file()
{
    std::string s;
    std::string path = TabLogs::logs_path + "dhcp-log-" + entries[selected] + ".log";
    std::ifstream file(path);

    log_content.clear();

    while (getline (file, s)) {
        //TODO: try to make it so that on long lines, the string is split into multiple and prefixed with "  " 2 spaces
        log_content.push_back(s);
    }
    
    file.close(); 
}

void TabLogs::refresh()
{
    load_entries();
    load_file();
}

