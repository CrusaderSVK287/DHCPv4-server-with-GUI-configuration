#ifndef __TAB_INSPECT_HPP__
#define __TAB_INSPECT_HPP__

#include <ftxui/component/component_base.hpp>
#include <string>
#include <vector>
#include "tab.hpp"
#include "database.hpp"

struct dhcp_message_str {
    std::string opcode;
    std::string htype;
    std::string hlen;
    std::string hops;

    std::string xid;

    std::string secs;
    std::string flags;

    std::string ciaddr;
    std::string yiaddr;
    std::string siaddr;
    std::string giaddr;
    std::string chaddr;

    std::string sname;
    std::string filename;

    std::string cookie;
    std::string options;

    std::string time_when_stored;
    std::string msg_type;
};

struct dhcp_option {
    int tag;
    int lenght;
    std::string name_str;
    std::string lenght_str;
    std::string value_str;
    std::string format;
};

class TabInspect : public UITab {
public:
    TabInspect();
    void refresh();

private:
    static constexpr const char* db_path = "/var/dhcp/database/";
    static std::vector<std::string> toggle_filter_entries;
    int toggle_filter_selected;

    std::string filter_content;
    std::string filter_content_last;

    std::vector<std::string> transaction_entries_xid;
    std::vector<std::string> transaction_entries_mac;
    // transaction menu
    int transaction_entry_selected;
    int transaction_entry_selected_last;
    std::vector<dhcp_message> loaded_transaction;
    std::vector<std::string> loaded_transaction_msg_types; // for convenience
    int loaded_transaction_selected;
    int loaded_transaction_selected_last;
    dhcp_message_str loaded_message;

    // For dhcp message details
    ftxui::Component msg_detail_menu;
    static std::vector<std::string> msg_detail_menu_entries;
    ftxui::Component msg_detail_tab;
    ftxui::Component msg_detail_values;
    ftxui::Component msg_detail_options;
    int msg_detail_selected;
    int msg_detail_selected_last;

    // for dhcp message options details
    std::vector<dhcp_option> loaded_dhcp_options;
    std::vector<std::string> dhcp_option_entries;
    dhcp_option selected_dhcp_option;
    int option_detail_selected;
    int option_detail_selected_last;

    void load_transactions();
    void load_selected_transaction();
    void load_selected_message();
    void load_selected_message_options();
    void load_selected_option();
};

#endif // !__TAB_INSPECT_HPP__

