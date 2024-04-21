#ifndef __TAB_CONFIG_HPP__
#define __TAB_CONFIG_HPP__

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
    CONFIG_COUNT // Add this to keep track of the number of configs
};

enum ConfType {
    NUMERIC,
    STRING,
    BOOLEAN,
    IP
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
    int load_config_file();
    int initialize();

    void set_config_server(ftxui::Component c);
    void set_config_pools(ftxui::Component c);
    void set_config_options(ftxui::Component c);
    void set_config_security(ftxui::Component c);
    // ~TabConfig();

private:

    static constexpr const char* default_config_path = "/etc/dhcp/config.json";

    static std::vector<std::string> config_menu_entries;
    static std::vector<std::string> boolean_toogle;
    ftxui::Component config_menu_tab;
    ftxui::Component config_menu_server;
    ftxui::Component config_menu_pools;
    ftxui::Component config_menu_options;
    ftxui::Component config_menu_security;
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
};

#endif // !__TAB_CONFIG_HPP__

