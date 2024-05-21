#include "tab_logs.hpp"
#include <filesystem>
#include <ftxui/component/component.hpp>
#include <ftxui/component/component_base.hpp>
#include <ftxui/dom/deprecated.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/dom/node.hpp>
#include <ftxui/screen/terminal.hpp>
#include <string>
#include <fstream>
#include "logger.hpp"

using namespace ftxui;
namespace fs = std::filesystem;

TabLogs::TabLogs()
{
    this->selected = 0;
    this->log_selected = 0;
    this->log_selected_last = -1;
    this->load_entries();

    this->tab_contents = Container::Horizontal({
        Menu({&this->entries, &this->selected}) | vscroll_indicator | yframe | yflex,
        Menu({&this->log_content, &this->log_selected}) | vscroll_indicator | yframe | xflex_shrink
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
    log_content.clear();

    if (entries.size() == 0)
        return;

    std::string s;
    std::string path = TabLogs::logs_path + "dhcp-log-" + entries[selected] + ".log";
    std::ifstream file(path);

    while (getline (file, s)) {
        int dimx = Terminal::Size().dimx - 25;
        log_content.push_back(s.substr(0, dimx));
        dimx -=4;
        for (unsigned i = dimx + 4; i < s.length(); i += dimx) {
            log_content.push_back("    " + s.substr(i, dimx));
        }
    }
    
    file.close();
}

void TabLogs::refresh()
{
    if (selected != log_selected_last) {
        load_entries();
        load_file();
        log_selected_last = selected;
    }
}

