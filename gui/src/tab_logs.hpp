#ifndef __TAB_LOGS_HPP__
#define __TAB_LOGS_HPP__

#include <ftxui/component/component_base.hpp>
#include <string>
#include <vector>
#include "tab.hpp"

class TabLogs : public UITab {
public:
    TabLogs();
    void refresh();

private:
    static constexpr std::string logs_path = "/var/log/dhcps/";

    int selected;
    int log_selected;
    std::vector<std::string> entries;
    std::vector<std::string> log_content;
    
    void load_entries();
    void load_file();
};

#endif // !__TAB_LOGS_HPP__

