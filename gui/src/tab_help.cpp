#include "tab_help.hpp"

#include <ftxui/component/component.hpp>
#include <ftxui/component/component_base.hpp>
#include <ftxui/dom/deprecated.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/dom/node.hpp>
#include <ftxui/screen/terminal.hpp>

using namespace ftxui;

std::vector<std::string> TabHelp::menu_entries = {
    "Movement",
    "Config",
    "Logs",
    "Inspect",
    "Leases",
    "Command"
};


TabHelp::TabHelp() 
{
    this->selected = 0;

    this->tab_contents = Container::Horizontal({
        Menu({&TabHelp::menu_entries, &this->selected}) | vscroll_indicator | yframe | yflex,
        Renderer([]{return separatorEmpty();}),
        Container::Tab({
            Container::Vertical({Renderer([]{
                return vbox({
                    text("Movement in this application is done in two ways"),
                    text("1. Using the mouse") | bold, 
                    text("  You can use mouse to click on tabs, buttons, menus etc"),
                    text("2. Using keyboard") | bold,
                    text("  Interface supports moving around elements using the arrow keys"),
                    text("  or using vim motion keybinds (hjkl). In addition you can use"),
                    text("  page down or page up buttons"), 
                    separatorEmpty(),
                    text("To exit the interface, use eighter the Exit button or send interupt signal"),
                    text("to the process (ctrl + C)")
                });
            })}),
            Container::Vertical({Renderer([]{
                return vbox({
                    text("Configuration of the server is done in the Config menu."),
                    hbox({
                            text("Please be aware that to apply the configuration, you "),
                            text(" MUST be root") | bold,
                    }),
                    text(" or have elevated priviledges. Otherwise, you cannot write to the config file"),
                    separatorEmpty(),
                    text("Config menu is separated to 4 sifferent sections"),
                    text("1. General") | bold,
                    text("    This section holds general server configuration. Apart from interface,"),
                    text("    it is not really needed to change these settings. For convenience, a short"),
                    text("    explanation is displayed on the side for currently focused item."),
                    text("2. Pools") | bold,
                    text("    In this section you create and edit address pools. To create new pool, you"),
                    text("    press the button on the bottom. Pressing remove pool deletes selected pool"),
                    text("3. Options") | bold,
                    text("    Here, DHCP options are configured. In order to edit the options, you have"),
                    text("    to first select the list of options in the first dropdown menu, and than"),
                    text("    select a particular option from the list in the seconds dropdown. You can add"),
                    text("    or remove an option using buttons at the bottom."),
                    text("    In addition to global dhcp option list, each pool has its own list. These"),
                    text("    lists work as an overwrite and are prefered over the global ones"),
                    text("4. Security") | bold,
                    text("    This section handles security measures. It consist of a few toggles and"),
                    text("    a list of client MAC addresses. You can add a new mac address using 'Add new entry'"),
                    text("    button. To remove an entry, erase it and press Delete empty entries"),
                    separatorEmpty(),
                    text("After you are finished configuring, dont forget to press the Apply button"),
                    text("at the bottom left of the screen, otherwise your configuration will be"),
                    text("forgotten and you will have to redo it all over again"),
                });
            })}),
            Container::Vertical({Renderer([]{
                return vbox({
                    text("Logs menu server the purpouse of displaying log files of the server."),
                    text("To view a log file, select its timestamp from the menu on the left, the log"),
                    text("will than be loaded and displayed on the right. Use movement up and down to"),
                    text("scroll the log"),
                });
            })}),
            Container::Vertical({Renderer([]{
                return vbox({
                    text("Inspect menu is used for in depth analysis of DHCP communications and transactions."),
                    text("It allows you to, in detail, inspect every single message exchnaged between a client"),
                    text("and the server."),
                    separatorEmpty(),
                    text("First select a transaction from the menu on the left. You can also use a filter"),
                    text("to filter out transactions with particular XID or that are associated with "),
                    text("particular client based on its MAC address. After you select a transactin, you "),
                    text("can select a particular message from the list to the right."),
                    separatorEmpty(),
                    text("The message detail panel is split into two main parts:"),
                    text("Message details tab:") | bold,
                    text("    Shows general information about the message, structure, namely data like"),
                    text("    xid, ciaddr, yiaddr, etc..."),
                    text("Options tab:") | bold,
                    text("    Shows DHCP options in the message. The list of options is show in the menu"),
                    text("    on the left side of the details panel by number. Selecting an option from"),
                    text("    this list will give you details on that option."),
                    separatorEmpty(),
                    text("If you want to delete the transaction entry from database, select it and pres"),
                    hbox({
                            text("Delete") | bold, 
                            text(" button. You need to be root or have elevated priviledges to do this."),
                    }),
                });
            })}),
            Container::Vertical({Renderer([]{
                return vbox({
                    text("Leases menu is similiar to the logs menu in that it allows you to see details"),
                    text("of the DHCP lease database. On selecting a pool of which you want to see leases,"),
                    text("You can select a particular leased address and its details will show up on the"),
                    text("panel to the left"),
                });
            })}),
            Container::Vertical({Renderer([]{
                return vbox({
                    text("Command menu acts as a communication medium with the server. Using commands"),
                    text("you are able to issue commands to the server or obtain information from the server."),
                    text("To issue a command, write it into the prompt (where it says 'Enter your command here')"),
                    text("and press enter. To see full list of commands, type in command 'help'"),
                });
            })}),
        }, &this->selected)
    });
}

