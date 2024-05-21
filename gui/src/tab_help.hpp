#ifndef __TAB_HELP_HPP__
#define __TAB_HELP_HPP__

#include <ftxui/component/component_base.hpp>
#include "tab.hpp"

class TabHelp : public UITab {
public:
    TabHelp();
    void refresh();

private:
    static std::vector<std::string> menu_entries;
    int selected;
};

#endif // !__TAB_HELP_HPP__
