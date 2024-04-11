#include "main_renderer.hpp"
#include <ftxui/dom/elements.hpp>
#include <ftxui/dom/node.hpp>
#include <ftxui/screen/screen.hpp>
#include <ftxui/screen/terminal.hpp>
#include <string>
#include <vector>
#include "ftxui/component/component.hpp" 
#include "ftxui/component/component_base.hpp" 
#include "ftxui/component/component_options.hpp"
#include "ftxui/component/event.hpp"              // for Event, Event::Custom
#include "ftxui/component/screen_interactive.hpp"  // for Component, ScreenInteractive
#include "ftxui/dom/elements.hpp" 
#include "version_info.hpp"

#include "tab_logs.hpp"

using namespace ftxui;

static Component _not_yet_implemented_tab(std::string s)
{
    return Renderer([s] {
            return text(std::format("Tab {} not yet implemented", s)) | bold | hcenter;
        });
}

int tui_loop()
{
    auto screen = ScreenInteractive::Fullscreen();

    int tab_index = 0;
    std::vector<std::string> tab_entries = {
        " help ", " logs ", " dhcp leases ", " config ", " idk ",
    };

    TabLogs tab_logs = TabLogs();

    auto tab_selection = Menu(&tab_entries, &tab_index, MenuOption::HorizontalAnimated()) | hcenter;
    auto tab_contents = Container::Tab({
        _not_yet_implemented_tab("help"),
        tab_logs.tab_contents,
        _not_yet_implemented_tab("leases"),
        _not_yet_implemented_tab("config"),
        _not_yet_implemented_tab("idk"),
        },
        &tab_index);

    auto main_container = Container::Vertical({
        Container::Horizontal({
            tab_selection,
        }),
        tab_contents,
    });
    
    auto main_renderer = Renderer(main_container, [&] {
        tab_logs.refresh();

        return vbox({
            hbox({
                text(TUI_VERSION) | bold | flex,
                text("DHCP config") | align_right | flex | bold,
            }),
            hbox({
                tab_selection->Render() | flex,
                // TODO: add some quit button
            }),
            tab_contents->Render() | flex,
        });
    });

    screen.Loop(main_renderer);

    return 0;
}

