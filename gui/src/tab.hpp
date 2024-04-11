#ifndef __TAB_HPP__
#define  __TAB_HPP__

#include <ftxui/component/component_base.hpp>

class UITab{
public:
    UITab();

    void refresh();

    ftxui::Component tab_contents;
private:
    
};

#endif // !__TAB_HPP__

