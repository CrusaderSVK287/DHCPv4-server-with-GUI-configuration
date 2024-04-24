#ifndef __TAB_CONFIG_HPP__
#define __TAB_CONFIG_HPP__

#include <cstddef>
#include <ftxui/component/component_base.hpp>
#include <string>
#include <vector>
#include "cJSON.h"
#include "tab.hpp"

enum ConfigName {
    CONF_INTERFACE,
    CONF_TICK_DELAY,
    CONF_CACHE_SIZE,
    CONF_TRANSACTION_DURATION,
    CONF_LEASE_EXPIRATION_CHECK,
    CONF_LOG_VERBOSITY,
    CONF_LEASE_TIME,
    CONF_DB_ENABLE,

    CONF_SEC_ACL_ENABLE,
    CONF_SEC_ACL_MODE,
    CONF_SEC_ACL_ENTRIES,
    CONFIG_COUNT // Add this to keep track of the number of configs
};

enum ConfType {
    NUMERIC,
    STRING,
    BOOLEAN,
    IP
};

struct DHCPOption {
    int tag;
    int lenght;
    std::string value;
};

struct DHCPOptionConfig {
    std::vector<DHCPOption> options;
    cJSON *json;
};

struct DHCPPool {
    std::string name;
    std::string start_addr;
    std::string end_addr;
    std::string subnet;
    cJSON *options_json;
};

struct ConfEntry {
    std::string name;
    std::string description;
    std::string json_path;
    ConfType   type;
    std::string val;
    int         val_i;
    std::string def_val;
    int         def_val_i;
    cJSON *json;
};

class TabConfig : public UITab {
public:
    TabConfig();
        
    void refresh();
    // loads config file
    int load_config_file();
    // initializes structures, vectors etc
    int initialize();
    // writes configuration to file
    int apply_settings();
    void update_acl_entries();
    void options_load();

private:

    static constexpr const char* default_config_path = "/etc/dhcp/config.json";

    static std::vector<std::string> config_menu_entries;
    static std::vector<std::string> boolean_toogle;
    static std::vector<std::string> enable_disable_toggle;
    static std::vector<std::string> blacklist_whitelist_toggle;
    ftxui::Component config_menu_tab;
    ftxui::Component config_menu_server;
    ftxui::Component config_menu_pools;
    ftxui::Component config_menu_options;

    ftxui::Component config_menu_security;
    ftxui::Component config_menu_security_acl_entries;
    int config_menu_selected;

    int config_server_selected;

    struct {
        cJSON *config;
        cJSON *server;
        cJSON *pools;
        cJSON *options;
        cJSON *security;
    } config_json;
    std::vector<ConfEntry> config_entries;

    // security
    std::vector<std::string> security_acl_entries;
    size_t security_acl_entries_size_last;

    // pools
    std::vector<DHCPPool> pools_pools;
    std::vector<std::string> pools_entries;
    int pools_pool_selected;

    // options
    std::vector<std::string> options_list;
    int options_list_selected;
    int options_list_selected_last;
    std::vector<DHCPOptionConfig> options_config_entries;
    DHCPOptionConfig options_loaded_list;
    std::vector<std::string> options_loaded_list_entries;
    int options_loaded_list_selected;
    int options_loaded_list_selected_last;
    ftxui::Component options_value_container;
    std::string options_loaded_type;
};

#endif // !__TAB_CONFIG_HPP__

