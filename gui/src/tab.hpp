#ifndef __TAB_HPP__
#define  __TAB_HPP__

#include <ftxui/component/component_base.hpp>
#include <string>

class UITab{
public:
    UITab();

    void refresh();

    ftxui::Component tab_contents;
    std::string error_msg;
private:
};

#endif // !__TAB_HPP__

