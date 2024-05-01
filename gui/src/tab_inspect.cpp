#include "tab_inspect.hpp"
#include "database.hpp"
#include <cstdint>
#include <filesystem>
#include <ftxui/component/component.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/dom/node.hpp>
#include <algorithm>
#include <ftxui/screen/terminal.hpp>
#include <netinet/in.h>
#include <string.h>
#include <string>
#include <arpa/inet.h>
#include "xtoy.hpp"
#include "logger.hpp"
#include "RFC-2132.hpp"

#define DHCP_OPTIONS_LEN 336 

using namespace ftxui;
namespace fs = std::filesystem;

std::vector<std::string> TabInspect::toggle_filter_entries = {
    " xid ",
    " MAC ",
};


std::vector<std::string> TabInspect::msg_detail_menu_entries = {
    " Details ",
    " Options "
};

static uint32_t uint32_from_uint8_array(const uint8_t* array) {
    return (static_cast<uint32_t>(array[0]) << 24) |
           (static_cast<uint32_t>(array[1]) << 16) |
           (static_cast<uint32_t>(array[2]) << 8) |
           static_cast<uint32_t>(array[3]);
}

static uint32_t uint32_from_uint8_array(const uint8_t* array, size_t lenght) {
    uint32_t result = 0;
    
    for (size_t i = 0; i < lenght; i++) {
        result = (result << 8) | array[i];
    }
    
    return result;
}

TabInspect::TabInspect() 
{ 
    // TODO: Maybe get some delete button for management
    this->toggle_filter_selected = 0;
    this->transaction_entry_selected = 0;
    this->loaded_transaction_selected = 0;
    this->loaded_transaction_selected_last = -1;
    this->transaction_entry_selected_last = -1;
    this->filter_content_last = "-----";
    this->msg_detail_selected = 0;
    this->msg_detail_selected_last = -1;
    this->option_detail_selected = 0;
    this->option_detail_selected_last = -1;
    this->msg_detail_values = Container::Vertical({
        Renderer([&]{
            return vbox({
                hbox({text("Message type:    ") | bold, text(loaded_message.msg_type)}),
                hbox({text("Time of storing: ") | bold, text(loaded_message.time_when_stored)}),
                hbox({text("XID:             ") | bold, text(loaded_message.xid)}),
                hbox({text("Magic cookie:    ") | bold, text(loaded_message.cookie)}) | size(HEIGHT, EQUAL, 2),

                hbox({text("opcode:          ") | bold, text(loaded_message.opcode)}),
                hbox({text("htype:           ") | bold, text(loaded_message.htype)}),
                hbox({text("hlen:            ") | bold, text(loaded_message.hlen)}),
                hbox({text("hops:            ") | bold, text(loaded_message.hops)}),

                hbox({text("secs:            ") | bold, text(loaded_message.secs)}),
                hbox({text("flags:           ") | bold, text(loaded_message.flags)}),

                hbox({text("ciaddr:          ") | bold, text(loaded_message.ciaddr)}),
                hbox({text("yiaddr:          ") | bold, text(loaded_message.yiaddr)}),
                hbox({text("siaddr:          ") | bold, text(loaded_message.siaddr)}),
                hbox({text("giaddr:          ") | bold, text(loaded_message.giaddr)}),
                hbox({text("chaddr:          ") | bold, text(loaded_message.chaddr)}),
            });
        }),
    });
    this->msg_detail_options = Container::Horizontal({
        Menu(&dhcp_option_entries, &option_detail_selected) | vscroll_indicator | yframe | yflex | size(WIDTH, EQUAL, 7),
        Renderer([&] {
            return vbox({
                hbox({text("Tag:    ") | bold, text(
                            std::to_string(selected_dhcp_option.tag) 
                            + " - " + selected_dhcp_option.name_str)}),
                hbox({text("Lenght: ") | bold, text(selected_dhcp_option.lenght_str)}),
                hbox({text("Value:  ") | bold, text(selected_dhcp_option.format)}),
                text(selected_dhcp_option.value_str)
            });
        }),
    });

    // message detail menu
    this->msg_detail_tab = Container::Tab({
        msg_detail_values,
        msg_detail_options
    }, &this->msg_detail_selected);
    this->msg_detail_menu = Menu(&TabInspect::msg_detail_menu_entries, &this->msg_detail_selected, 
            MenuOption::Horizontal());
    
    tab_contents = Container::Vertical({
        Container::Horizontal({
            Renderer([&] {return text(" Filter by: ") | bold;}),
            Toggle(&TabInspect::toggle_filter_entries, &this->toggle_filter_selected),
        }),
        Container::Horizontal({
            Renderer([&] {return text(" Value:     ") | bold;}),
            Input(&this->filter_content, "Enter MAC or XID here"),
        }),
        Renderer([] {return separator();}),

        Container::Horizontal({
            // transaction menu
            Menu(&transaction_entries_xid, &transaction_entry_selected) 
                | vscroll_indicator | yframe | yflex | size(WIDTH, EQUAL, 12),
            Renderer([] {return separator();}),

            // messages menu
            Menu(&loaded_transaction_msg_types, &loaded_transaction_selected)
                | vscroll_indicator | yframe | yflex | size(WIDTH, EQUAL, 12),
            Renderer([] {return separator();}),

            // window with message details
            Container::Vertical({
                this->msg_detail_menu | flex_grow | center | size(WIDTH, EQUAL, Terminal::Size().dimx - 24),
                // Renderer([]{return separator();}),
                this->msg_detail_tab
            })
        }) | flex,
    }) | flex;
}

void TabInspect::refresh()
{
    if (filter_content.compare(filter_content_last) != 0 ||
        transaction_entry_selected_last != transaction_entry_selected) {
        load_transactions();
        load_selected_transaction();
        loaded_transaction_selected_last = -1;
    }

    load_selected_message();
    load_selected_option();
}


void TabInspect::load_selected_message_options()
{
    loaded_dhcp_options.clear();
    dhcp_option_entries.clear();

    if (loaded_transaction_selected == loaded_transaction_selected_last)
        return;

    dhcp_option o = {0};

    uint8_t *options = loaded_transaction[loaded_transaction_selected].options;
    
    int i = 0;
    while (i < DHCP_OPTIONS_LEN) {
        o.tag = options[i];
        o.lenght = options[++i];
        o.lenght_str = std::to_string(o.lenght);
        o.name_str = DHCPv4_tag_to_full_name((DHCPv4_Options)o.tag);

        i++;
        o.value_str.clear();
        switch (dhcp_option_tag_to_type(o.tag)) {
            case DHCP_OPTION_STRING: {
                std::ostringstream convert;
                for (int j = i; j < i + o.lenght; j++) {
                    convert << (char)options[j];
                }
                o.value_str = convert.str();
                o.format = "String";
                break;
            }
            case DHCP_OPTION_IP: {
                uint32_t ip = uint32_from_uint8_array(options + i);
                o.value_str = uint32_to_ipv4_address(ip);
                o.format = "IP Address";
                break;
            }
            case DHCP_OPTION_BIN: { 
                for (int j = i; j < i + o.lenght; j++) {
                    o.value_str += std::to_string(options[j]) + " ";
                }
                o.format = "Binary";
                break;
            }
            case DHCP_OPTION_BOOL: {
                o.value_str = (options[i]) ? "TRUE" : "FALSE";
                o.format = "Boolean";
                break;
            }
            case DHCP_OPTION_NUMERIC: {
                uint32_t n = uint32_from_uint8_array(options + i, o.lenght);
                o.value_str = std::to_string(n);
                o.format = "Number";
                break;
            }
            default:
                o.format = "ERROR";
                o.value_str = "ERROR";
                break;
        }

        i += o.lenght;
        if (o.tag == 0)
            continue;
        if (o.tag == 255)
            break;

        loaded_dhcp_options.push_back(o);
        dhcp_option_entries.push_back(std::to_string(o.tag));
    }
}

void TabInspect::load_selected_message()
{
    if (loaded_transaction_selected == loaded_transaction_selected_last)
        return;

    if (transaction_entries_xid.size() == 0) {
        loaded_message.opcode = "";
        loaded_message.htype = "";
        loaded_message.hlen = "";
        loaded_message.hops = "";
        loaded_message.xid = "";
        loaded_message.secs = "";
        loaded_message.flags = "";
        loaded_message.ciaddr = "";
        loaded_message.yiaddr = "";
        loaded_message.siaddr = "";
        loaded_message.giaddr = "";
        loaded_message.chaddr = "";
        loaded_message.sname = "";
        loaded_message.filename = "";
        loaded_message.cookie = "";
        loaded_message.options = "";
        loaded_message.msg_type = "";
        loaded_message.time_when_stored = "";
        return;
    }
    dhcp_message &m = loaded_transaction[loaded_transaction_selected];

    loaded_message.msg_type = m.msg_type;
    {
        std::time_t time = m.time_when_stored;
        std::tm* t = std::gmtime(&time);
        std::stringstream ss; ss << std::put_time(t, "%Y-%m-%d %I:%M:%S %p");
        loaded_message.time_when_stored = ss.str();
    }
    {
        std::stringstream ss; 
        ss << "0x" << std::setw(8) << std::setfill('0') << std::hex << m.xid;
        loaded_message.xid = ss.str();
    }
    loaded_message.opcode = (std::stringstream() << static_cast<int>(m.opcode)).str();
    loaded_message.htype = (std::stringstream() << static_cast<int>(m.htype)).str();
    loaded_message.hlen = (std::stringstream() << static_cast<int>(m.hlen)).str();
    loaded_message.hops = (std::stringstream() << static_cast<int>(m.hops)).str();
    {
        std::stringstream ss; ss << m.secs;
        loaded_message.secs = ss.str();
    }
    {
        std::stringstream ss; ss << m.flags;
        loaded_message.flags = ss.str();
    }
    {
        std::stringstream ss; ss << uint32_to_ipv4_address(m.ciaddr);
        loaded_message.ciaddr = ss.str();
    }
    {
        std::stringstream ss; ss << uint32_to_ipv4_address(m.yiaddr);
        loaded_message.yiaddr = ss.str();
    }
    {
        std::stringstream ss; ss << uint32_to_ipv4_address(m.siaddr);
        loaded_message.siaddr = ss.str();
    }
    {
        std::stringstream ss; ss << uint32_to_ipv4_address(m.giaddr);
        loaded_message.giaddr = ss.str();
    }
    {
        std::stringstream ss; ss << uint8_array_to_mac(m.chaddr);
        loaded_message.chaddr = ss.str();
    }
    {
        std::stringstream ss; ss << "0x" << std::setw(8) << std::setfill('0') << std::hex << m.cookie;
        loaded_message.cookie = ss.str();
    }

    option_detail_selected = 0;
    option_detail_selected_last = -1;
    load_selected_message_options();
    loaded_transaction_selected_last = loaded_transaction_selected;
}


void TabInspect::load_selected_transaction()
{
    if (transaction_entries_xid.size() == 0)
        return;

    loaded_transaction.clear();
    loaded_transaction_msg_types.clear();
    
    loaded_transaction = database_load_entry(TabInspect::db_path + 
            transaction_entries_xid[transaction_entry_selected]  +
            "_"                                                  +
            transaction_entries_mac[transaction_entry_selected]  + 
            ".dhcp");

    for (auto & entry : loaded_transaction) {
        strcat(entry.msg_type, " ");
        loaded_transaction_msg_types.push_back(entry.msg_type);
    }

}

void TabInspect::load_transactions()
{
    transaction_entries_xid.clear();
    transaction_entries_mac.clear();

    for (auto &entry : fs::directory_iterator(TabInspect::db_path)) {
        if (filter_content.length() > 1) {
            transaction_entry_selected = 0;

            if (toggle_filter_selected == 0) {
                // filter by xid
                if (entry.path().string().substr(19, 8).find(filter_content) == entry.path().string().npos) {
                    continue;
                }
            } else {
                // filter by mac
                std::string filter = filter_content;
                filter.erase(std::remove_if(filter.begin(), filter.end(), 
                             [](char c) { return c == ':'; }), filter.end());
                if (entry.path().string().substr(28, 12).find(filter) == entry.path().string().npos) {
                    continue;
                }
            }
        }

        transaction_entries_xid.push_back(entry.path().string().substr(19, 8));
        transaction_entries_mac.push_back(entry.path().string().substr(28, 12));
    }

    filter_content_last = filter_content;
    transaction_entry_selected_last = transaction_entry_selected;
}

void TabInspect::load_selected_option()
{
    if (transaction_entries_xid.size() == 0) {
        selected_dhcp_option.value_str  = "";
        selected_dhcp_option.name_str   = "";
        selected_dhcp_option.lenght_str = "";
        selected_dhcp_option.tag        = 0;
        selected_dhcp_option.lenght     = 0;
        selected_dhcp_option.format     = "";
        return;
    }

    if (option_detail_selected_last == option_detail_selected)
        return;

    dhcp_option &o = loaded_dhcp_options[option_detail_selected];

    selected_dhcp_option.value_str  = o.value_str;
    selected_dhcp_option.name_str   = o.name_str;
    selected_dhcp_option.lenght_str = o.lenght_str;
    selected_dhcp_option.tag        = o.tag;
    selected_dhcp_option.lenght     = o.lenght;
    selected_dhcp_option.format     = o.format;

    option_detail_selected_last = option_detail_selected;
}

