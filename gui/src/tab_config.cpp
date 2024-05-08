#include "tab_config.hpp"
#include "cJSON.h"
#include "ftxui/component/component.hpp"
#include "ftxui/dom/elements.hpp"
#include "xtoy.hpp"
#include "logger.hpp"
#include "RFC-2132.hpp"

#include <climits>
#include <cerrno> // For errno
#include <cstring> // For strerrorinclude <cstdio>
#include <fstream>
#include <ftxui/dom/deprecated.hpp>
#include <ftxui/dom/node.hpp>
#include <ftxui/screen/color.hpp>
#include <iostream>
#include <ostream>
#include <stdexcept>
#include <string>

using namespace ftxui;
// TODO: nejaky check na zaciatku ci som root
std::vector<std::string> TabConfig::config_menu_entries = {
    "General",
    "Pools",
    "Options",
    "Security",
};

std::vector<std::string> TabConfig::boolean_toogle = {
    "False",
    "True",
};
std::vector<std::string> TabConfig::enable_disable_toggle = {
    "Disabled ",
    "Enabled"
};
std::vector<std::string> TabConfig::blacklist_whitelist_toggle = {
    "Whitelist",
    "Blacklist"
};

static void clearArray(cJSON *array) {
    cJSON *element = cJSON_GetArrayItem(array, 0);
    while (element != NULL) {
        cJSON_DeleteItemFromArray(array, 0);
        element = cJSON_GetArrayItem(array, 0);
    }
}

TabConfig::TabConfig()
{
    this->initialize();

    this->load_config_file();
    this->config_server_selected = 0;
    this->pools_pool_selected = 0;
    this->options_list_selected = 0;
    this->options_list_selected_last = -1;

    this->options_loaded_list_selected = 0;
    this->options_loaded_list_selected_last = -1;
    this->pools_pool_selected_last = -1;
    // We require the initialize function to be run before doing this.
    this->config_menu_server = Container::Horizontal({
        Renderer([&] {return vbox({
                text(this->config_entries[CONF_INTERFACE].name) | bold,
                text(this->config_entries[CONF_TICK_DELAY].name) | bold,
                text(this->config_entries[CONF_CACHE_SIZE].name) | bold,
                text(this->config_entries[CONF_TRANSACTION_DURATION].name) | bold,
                text(this->config_entries[CONF_LEASE_EXPIRATION_CHECK].name) | bold,
                text(this->config_entries[CONF_LOG_VERBOSITY].name) | bold,
                text(this->config_entries[CONF_LEASE_TIME].name) | bold,
                text(this->config_entries[CONF_DB_ENABLE].name) | bold,
            });
        }),
        Renderer([] {return separatorEmpty();}),
        Container::Vertical({
            Input(&this->config_entries[CONF_INTERFACE].val),
            Input(&this->config_entries[CONF_TICK_DELAY].val),
            Input(&this->config_entries[CONF_CACHE_SIZE].val),
            Input(&this->config_entries[CONF_TRANSACTION_DURATION].val),
            Input(&this->config_entries[CONF_LEASE_EXPIRATION_CHECK].val),
            Input(&this->config_entries[CONF_LOG_VERBOSITY].val),
            Input(&this->config_entries[CONF_LEASE_TIME].val),
            Toggle(&TabConfig::boolean_toogle ,&this->config_entries[CONF_DB_ENABLE].val_i),
        })
        | CatchEvent([&] (Event event) {
            Component &inputs = this->config_menu_server->ChildAt(2)->ChildAt(0);
            for (size_t i = 0; i < inputs->ChildCount(); i++) {
                if (inputs->ChildAt(i)->Focused()) {
                    this->config_server_selected = i;
                    break;
                }
            }
            return false;
        }),
        Renderer([] {return separatorEmpty();}),
        Renderer([&] {return vbox({
                        text(this->config_entries[this->config_server_selected].name + ":"),
                        paragraphAlignLeft(this->config_entries[this->config_server_selected].description)
                    });})
    });
    
    this->pools_value_container = Container::Vertical({});
    this->config_menu_pools = Container::Vertical({
        Container::Horizontal({
            Menu(&this->pools_entries, &this->pools_pool_selected)
                | vscroll_indicator | yframe | yflex | size(WIDTH, GREATER_THAN, 10),
            Renderer([] {return hbox({separatorEmpty(), separator(), separatorEmpty()});}),
            Container::Vertical({
                // tmp
                Renderer([&] {
                    return vbox({
                        text("Pool name") | bold,   
                        text("Start address") | bold,   
                        text("End address") | bold,   
                        text("Subnet mask") | bold,   
                    });
                })
            }),
            Renderer([]{return separatorEmpty();}),
            this->pools_value_container,
        }) | flex,
        Renderer([]{return separator();}),
        Container::Horizontal({
            Button("+ Add pool", [&] {pool_ctl(false);}),
            Button("- Remove pool", [&] {pool_ctl(true);}),
            // TODO: add some button to go to options from pool and to pools from options for convenience
        })
    }),

    this->options_value_container = Container::Horizontal({});
    this->config_menu_options = Container::Vertical({
        Container::Horizontal({
            Dropdown(&this->options_list, &this->options_list_selected),
            Dropdown(&this->options_loaded_list_entries, &this->options_loaded_list_selected),
        }),
        Renderer([&] {
                return vbox({
                    separator(),
                    text("Option is of type " + this->options_loaded_type) | bold,
                    separatorEmpty()    
                });
        }),
        Container::Horizontal({
            Renderer([] {return text("Option value: ");}),
            this->options_value_container,
        }) | yflex_grow,
        Container::Horizontal({
            Button(" Add ", [&] {this->dhcp_option_ctl(false);}),
            Renderer([] {return text("/");}) | vcenter,
            Button("Remove", [&] {this->dhcp_option_ctl(true);}),
            Renderer([] {return text("option number ");}) | vcenter,
            Input(&this->dhcp_option_ctl_input, "...") | vcenter,
        }),
    });

    this->config_menu_security_acl_entries = Container::Vertical({});
    this->config_menu_security = Container::Vertical({
        Container::Horizontal({
            Renderer([&] {return vbox({
                    text(this->config_entries[CONF_SEC_ACL_ENABLE].name) | bold,
                    text(this->config_entries[CONF_SEC_ACL_MODE].name) | bold,
                });
            }),
            Renderer([] {return separatorEmpty() | size(WIDTH, EQUAL, 3);}),
            Container::Vertical({
                Toggle(&TabConfig::enable_disable_toggle, &this->config_entries[CONF_SEC_ACL_ENABLE].val_i),
                Toggle(&TabConfig::blacklist_whitelist_toggle, &this->config_entries[CONF_SEC_ACL_MODE].val_i),
            }),
        }),
        Renderer([] {return vbox({
                        separatorEmpty(),
                        text("Enter your ACL entries below:") | bold,
                    });
        }),
        this->config_menu_security_acl_entries | vscroll_indicator | yframe | yflex,
        Renderer([] {return separatorEmpty();}),

        Container::Horizontal({
            Button("+ Add new entry", [&] {this->security_acl_entries.push_back("00:00:00:00:00:00");}),
            Button("- Delete empty entries", [&] {
                    this->security_acl_entries.erase(std::remove(
                                            this->security_acl_entries.begin(), 
                                            this->security_acl_entries.end(), 
                                            ""), 
                                          this->security_acl_entries.end());
                    }),
        })
    });
    
    this->config_menu_selected = 0;

    this->config_menu_tab = Container::Tab({
        this->config_menu_server,
        this->config_menu_pools,
        this->config_menu_options,
        this->config_menu_security
    }, &this->config_menu_selected);

    this->tab_contents = Container::Vertical({ 
        Container::Horizontal({
            Container::Vertical({
                Menu(&TabConfig::config_menu_entries, &this->config_menu_selected) | size(WIDTH, EQUAL, 11)  | vscroll_indicator | yframe | yflex,
                Button("Apply", [&] {this->apply_settings();}),
            
            }),
            Renderer([]{return separator();}),
            this->config_menu_tab,
        }) | flex,
        Renderer([&] {return paragraph(this->error_msg) | bgcolor(Color::Red);})
    });

    this->security_acl_entries_size_last = INT_MAX;
}

void TabConfig::refresh()
{
    // error_msg = "";
    update_acl_entries();
    options_load();
    pools_refresh();
}

static void load_dhcp_config(DHCPOptionConfig &c) 
{
    if (!c.json) 
        return;

    c.options.clear();

    cJSON *e;
    cJSON_ArrayForEach(e, c.json) {
        int tag = cJSON_GetNumberValue(cJSON_GetObjectItem(e, "tag"));
        int lenght = cJSON_GetNumberValue(cJSON_GetObjectItem(e, "lenght"));

        DHCPOption o = {
            .tag = tag,
            .lenght = lenght,
        };

        switch (dhcp_option_tag_to_type(tag)) {
            case DHCP_OPTION_IP:
            case DHCP_OPTION_STRING:
                o.value = cJSON_GetStringValue(cJSON_GetObjectItem(e, "value"));
                break;
            case DHCP_OPTION_NUMERIC:
            case DHCP_OPTION_BOOL: {
                std::ostringstream oss;
                oss << std::fixed << std::setprecision(0) << cJSON_GetNumberValue(cJSON_GetObjectItem(e, "value"));
                o.value = oss.str();
                break;
            }
            case DHCP_OPTION_BIN:
            default:
                break;
        }

        if (o.value.length() != 0) {
            c.options.push_back(o);
        }
    }
}

int TabConfig::load_config_file()
{
    std::ifstream file(TabConfig::default_config_path, std::ios::binary);
    if (file) {
        file.seekg(0, std::ios::end);
        std::streampos file_size = file.tellg();
        file.seekg(0, std::ios::beg);

        std::unique_ptr<char[]> buffer(new char[file_size]);
        if (!buffer) {
            file.close();
            return -1;
        }

        file.read(buffer.get(), file_size);

        config_json.config = cJSON_Parse(buffer.get());
        if (!config_json.config) {
            log("Erorr: failed to parse JSON");
            return -1;
        }

        config_json.server   = cJSON_GetObjectItem(config_json.config, "server"  );
        config_json.pools    = cJSON_GetObjectItem(config_json.config, "pools"   );
        config_json.options  = cJSON_GetObjectItem(config_json.config, "options" );
        config_json.security = cJSON_GetObjectItem(config_json.config, "security");
    } else {
        config_json.config   = cJSON_CreateObject();
        config_json.server   = cJSON_AddObjectToObject(config_json.config, "server");
        config_json.pools    = cJSON_AddArrayToObject(config_json.config, "pools");
        config_json.options  = cJSON_AddArrayToObject(config_json.config, "options");
        config_json.security = cJSON_AddObjectToObject(config_json.config, "security");
    }

    // -------------------------
    //      server config
    // -------------------------

    for (int i = CONF_INTERFACE; i <= CONF_DB_ENABLE; i++) {
        ConfEntry &entry = config_entries[i];
        entry.json = cJSON_GetObjectItem(config_json.server, entry.json_path.c_str());

        /* If the configuration entry wasnt present in the config file before, create it now */
        if (!entry.json) {
            switch (entry.type) {
                case STRING:
                case IP:
                    if (!cJSON_AddStringToObject(config_json.server, entry.json_path.c_str(), entry.def_val.c_str())) {
                        log("Failed to add " + entry.json_path + " to object");
                    }
                    break;
                case NUMERIC:
                    if (!cJSON_AddNumberToObject(config_json.server, entry.json_path.c_str(), entry.def_val_i)) {
                        log("Failed to add " + entry.json_path + " to object");
                    }
                    break;
                case BOOLEAN:
                    if(!cJSON_AddBoolToObject(config_json.server, entry.json_path.c_str(), entry.def_val_i)){
                        log("Failed to add " + entry.json_path + " to object");
                    }
                    break;
                default:
                    continue;
            }

            entry.json = cJSON_GetObjectItem(config_json.server, entry.json_path.c_str());
            continue;
        }

        switch (entry.type) {
            case STRING: {
                if (!cJSON_IsString(entry.json)) {
                    log(entry.name + " Is not a string");
                    return -1;
                }
                entry.val = cJSON_GetStringValue(entry.json);
                break;
            }
            case IP: {
                if (!cJSON_IsString(entry.json)) {
                    log(entry.name + " Is not a string");
                    return -1;
                }   
                entry.val = cJSON_GetStringValue(entry.json);
                // Check if the string is valid IP address
                if (ipv4_address_to_uint32(entry.val.c_str()) == 0)
                    return -1;
                break;
            }
            case NUMERIC: {
                if (!cJSON_IsNumber(entry.json)) {
                    log(entry.name + " Is not a number");
                    return -1;
                }
                std::ostringstream oss;
                oss << std::fixed << std::setprecision(0) << cJSON_GetNumberValue(entry.json);
                entry.val = oss.str();
                break;
            }
            case BOOLEAN: {
                if (!cJSON_IsBool(entry.json)) {
                    log(entry.name + " Is not a bool");
                    return -1;
                }
                entry.val_i = cJSON_IsTrue(entry.json);
                break;
            }
            default:
                break;
        }
    }

    // -------------------------------
    //        security config 
    // -------------------------------

    ConfEntry &acl_enable = config_entries[CONF_SEC_ACL_ENABLE];
    ConfEntry &acl_mode = config_entries[CONF_SEC_ACL_MODE];
    ConfEntry &acl_list = config_entries[CONF_SEC_ACL_ENTRIES];
    acl_enable.json = cJSON_GetObjectItem(config_json.security, acl_enable.json_path.c_str());
    acl_mode.json = cJSON_GetObjectItem(config_json.security, acl_mode.json_path.c_str());
    acl_list.json = cJSON_GetObjectItem(config_json.security, acl_list.json_path.c_str());

    if (!acl_enable.json) {
        acl_enable.json = cJSON_AddBoolToObject(config_json.security, acl_enable.json_path.c_str(), acl_enable.def_val_i);
    }
    if (!acl_mode.json) {
        acl_mode.json = cJSON_AddBoolToObject(config_json.security, acl_mode.json_path.c_str(), acl_mode.def_val_i);
    }
    if (!acl_list.json) {
        acl_list.json = cJSON_AddArrayToObject(config_json.security, acl_list.json_path.c_str());
    }

    if (!cJSON_IsBool(acl_enable.json)) {
        log("ACL enable is not bool");
        return -1;
    }
    if (!cJSON_IsBool(acl_mode.json)) {
        log("ACL mode is not bool");
        return -1;
    }
    if (!cJSON_IsArray(acl_list.json)) {
        log("ACL entries is not an array");
        return -1;
    }

    security_acl_entries.clear();

    cJSON *e;
    cJSON_ArrayForEach(e, acl_list.json) {
        security_acl_entries.push_back(e->valuestring);
    }
    security_acl_entries_size_last = security_acl_entries.size();

    // -----------------------
    //        Pools
    // -----------------------

    pools_entries.clear();
    pools_pools.clear();

    e = NULL;
    cJSON_ArrayForEach(e, config_json.pools) {
        DHCPPool pool = {
            .name = cJSON_GetStringValue(cJSON_GetObjectItem(e, "name")),
            .start_addr = cJSON_GetStringValue(cJSON_GetObjectItem(e, "start")),
            .end_addr = cJSON_GetStringValue(cJSON_GetObjectItem(e, "end")),
            .subnet = cJSON_GetStringValue(cJSON_GetObjectItem(e, "subnet")),
            .options_json = cJSON_GetObjectItem(e, "options")
        };

        if (!pool.options_json) {
            pool.options_json = cJSON_AddArrayToObject(e, "options");
        }

        if (pool.name.length() == 0 || ipv4_address_to_uint32(pool.start_addr.c_str()) == 0
                                    || ipv4_address_to_uint32(pool.end_addr.c_str()) == 0
                                    || ipv4_address_to_uint32(pool.subnet.c_str()) == 0)
            return -1;

        pools_pools.push_back(pool);
        pools_entries.push_back(pool.name);
    }

    // ----------------------
    //      options
    // ----------------------
    
    options_list.clear();
    options_list.push_back("Global");
    for(auto &entry : pools_entries) {
        options_list.push_back(entry);
    }

    // Global options

    DHCPOptionConfig options_global = {
        .json = config_json.options
    };
    load_dhcp_config(options_global);
    options_config_entries.push_back(options_global);

    for (auto &entry : pools_pools) {
        DHCPOptionConfig options_pool = {
            .json = entry.options_json
        };
        load_dhcp_config(options_pool);
        options_config_entries.push_back(options_pool);
    }

    return 0;
}

void TabConfig::update_acl_entries()
{
    if (security_acl_entries.size() == security_acl_entries_size_last)
        return;

    config_menu_security_acl_entries->DetachAllChildren();
    for(auto &entry : security_acl_entries) {
        config_menu_security_acl_entries->Add({
            Container::Horizontal({
                Input(&entry),
                Renderer([]{return text(" <");}),
            })
        });
    }

    security_acl_entries_size_last = security_acl_entries.size();
}

int TabConfig::initialize()
{
    // config_entries.clear();

    // Initialize 'Interface' entry.
    ConfEntry c_interface = {
        .name = "Interface",
        .description = "Interface on which server will be listening for DHCP client requests",
        .json_path = "interface",
        .type = STRING,
        .val = "",
        .def_val= "",
    };
    config_entries.push_back(c_interface);

    // Initialize 'Tick Delay' entry.
    ConfEntry c_tick_delay = {
        .name = "Tick Delay",
        .description = "Interval (in milliseconds) between each server tick. Longer interval can lower CPU load but might cause the server to be unresponsive.",
        .json_path = "tick_delay",
        .type = NUMERIC,
        .val = "1000",
        .def_val= "1000",
    };
    config_entries.push_back(c_tick_delay);

    // Initialize 'Cache Size' entry.
    ConfEntry c_cache_size = {
        .name = "Cache Size",
        .description = "Maximum number of items stored in the server cache. The higher the size, the more clients can be handled at a time. Lower values provide protection against DHCP starvation attack.",
        .json_path = "cache_size",
        .type = NUMERIC,
        .val = "25",
        .def_val= "25",
    };
    config_entries.push_back(c_cache_size);

    // Initialize 'Transaction Duration' entry.
    ConfEntry c_transaction_duration = {
        .name = "Transaction Duration",
        .description = "Duration (in seconds) of a DHCP transaction stored in cache. Similiar to the cache size, higher values will provide protection against DHCP starvation attacks by making that transaction last in cache for longer.",
        .json_path = "transaction_duration",
        .type = NUMERIC,
        .val = "60",
        .def_val = "60",
    };
    config_entries.push_back(c_transaction_duration);

    // Initialize 'Lease Expiration Check' entry.
    ConfEntry c_lease_expiration_check = {
        .name = "Lease Expiration Check",
        .description = "Interval (in seconds) between each lease expiration check. Server periodically checks for expired DHCP leases. Lower values mean that expired addresses are returned to pool sooner.",
        .json_path = "lease_expiration_check",
        .type = NUMERIC,
        .val = "60",
        .def_val = "60",
    };
    config_entries.push_back(c_lease_expiration_check);

    // Initialize 'Log Verbosity' entry.
    ConfEntry c_log_verbosity = {
        .name = "Log Verbosity",
        .description = "Level of verbosity for server logging. 1 - Only critical errors, 2 - All errors, 3 - Warnings, 4 (default) messages note taking look at, 5 - very verbose, all messages present.",
        .json_path = "log_verbosity",
        .type = NUMERIC,
        .val = "4",
        .def_val= "4",
    };
    config_entries.push_back(c_log_verbosity);

    // Initialize 'Lease Time' entry.
    ConfEntry c_lease_time = {
        .name = "Lease Time",
        .description = "Duration (in seconds) of a DHCP lease. This is a global setting for lease duration. WARNING: setting this option in options section of config will have NO EFFECT.",
        .json_path = "lease_time",
        .type = NUMERIC,
        .val = "43200",
        .def_val = "43200",
    };
    config_entries.push_back(c_lease_time);

    // Initialize 'DB Enable' entry.
    ConfEntry c_db_enable = {
        .name = "Database Enable",
        .description = "Enable or disable database usage. Database stores all transactions that are handled by the server and stored in /var/dhcp/database. Transactions can be viewed in detail in Inspect tab",
        .json_path = "db_enable",
        .type = BOOLEAN,
        .val_i = 1,
        .def_val_i = 1,
    };
    config_entries.push_back(c_db_enable);

    // -------------------------------------
    //           security configs 
    // ------------------------------------

    ConfEntry c_acl_enable = {
        .name = "ACL enable",
        .description = "Enable or disable ACL security feature. If enabled, each client is decided whether to be served or not based on ACL mode and entries in the list.",
        .json_path = "acl_enable",
        .type = BOOLEAN,
        .val_i = 1,
        .def_val_i = 1,
    };
    config_entries.push_back(c_acl_enable);

    ConfEntry c_acl_mode = {
        .name = "ACL mode",
        .description = "Sets the mode of ACL. Blacklist mode means that all clients specified in the list will be declined service. Whitelist mode means that the clients in the list will be served, and all others will be denied.",
        .json_path = "acl_blacklist",
        .type = BOOLEAN,
        .val_i = 1,
        .def_val_i = 1,
    };
    config_entries.push_back(c_acl_mode);

    // Not really used, just for completion sake and JSON handling. Values are stored in a vector
    ConfEntry c_acl_entries = {
        .name = "ACL entries",
        .description = "Clients in ACL list",
        .json_path = "acl_entries",
        .type = STRING,
        .val = "",
        .def_val = "",
    };
    config_entries.push_back(c_acl_entries);

    if (config_entries.size() != CONFIG_COUNT)
        throw std::runtime_error("TabConfig inproperly initialized");

    return 0;
}

static void apply_options(DHCPOptionConfig &o)
{

    clearArray(o.json);

    for (auto &entry : o.options) {
        cJSON *opt = cJSON_CreateObject();

        // Set correct option sizes, particullarly for string options
        switch(dhcp_option_tag_to_type(entry.tag)) {
            case DHCP_OPTION_BIN:
            case DHCP_OPTION_STRING:
                entry.lenght = entry.value.length();
                break;
            case DHCP_OPTION_IP:
                if (ipv4_address_to_uint32(entry.value.c_str()) == 0) {
                    log("Bad IP format");
                    return;
                }
                entry.lenght = 4;
                break;
            case DHCP_OPTION_BOOL:
                entry.lenght = 1;
                break;
            case DHCP_OPTION_NUMERIC:
                if (entry.lenght == 0)
                    entry.lenght = 4;
                break;
            default:
                break;
        }

        cJSON_AddNumberToObject(opt, "tag", entry.tag);
        cJSON_AddNumberToObject(opt, "lenght", entry.lenght);
        switch(dhcp_option_tag_to_type(entry.tag)) {
            case DHCP_OPTION_STRING:
            case DHCP_OPTION_IP:
            case DHCP_OPTION_BIN:
                cJSON_AddStringToObject(opt, "value", entry.value.c_str());
                break;

            case DHCP_OPTION_BOOL:
                cJSON_AddBoolToObject(opt, "value", std::stoi(entry.value));
                break;
            case DHCP_OPTION_NUMERIC:
                cJSON_AddNumberToObject(opt, "value", std::stoi(entry.value));
                break;
            default:
                break;
        }

        if (dhcp_option_tag_to_type(entry.tag) != DHCP_OPTION_BIN)
            cJSON_AddItemToArray(o.json, opt);
    }
}

int TabConfig::apply_settings()
{
    ConfEntry &entry = config_entries[0];
    for (int i = 0; i <= CONF_DB_ENABLE; i++) {
        entry = config_entries[i];
        entry.json = cJSON_GetObjectItem(config_json.server, entry.json_path.c_str());
        if (entry.json) {
            switch (entry.type) {
                case STRING: {
                    if (!cJSON_IsString(entry.json)) {
                        log(entry.name + " Is not a string");
                        return -1;
                    }
                    cJSON_SetValuestring(entry.json, entry.val.c_str());
                    break; // Don't forget to break after each case
                }
                case IP: {
                    if (!cJSON_IsString(entry.json)) {
                        log(entry.name + " Is not a string");
                        return -1;
                    }   
                    cJSON_SetValuestring(entry.json, entry.val.c_str());
                    // No need to check IP validity here as we're just updating the value
                    break;
                }
                case NUMERIC: {
                    if (!cJSON_IsNumber(entry.json)) {
                        log(entry.name + " Is not a number");
                        return -1;
                    }
                    cJSON_SetNumberValue(entry.json, std::stoi(entry.val));
                    break;
                }
                case BOOLEAN: {
                    if (!cJSON_IsBool(entry.json)) {
                        log(entry.name + " Is not a bool");
                        return -1;
                    }
                    cJSON_SetBoolValue(entry.json, entry.val_i);
                    break;
                }
                default:
                    break;
            }
        }
    }

    // ------------------------
    //       security 
    // ------------------------
    this->security_acl_entries.erase(std::remove(
                                            this->security_acl_entries.begin(), 
                                            this->security_acl_entries.end(), 
                                            ""), 
                                          this->security_acl_entries.end());
    update_acl_entries();

    ConfEntry &acl_enable = config_entries[CONF_SEC_ACL_ENABLE];
    ConfEntry &acl_mode = config_entries[CONF_SEC_ACL_MODE];
    ConfEntry &acl_list = config_entries[CONF_SEC_ACL_ENTRIES];
    cJSON_SetBoolValue(acl_enable.json, acl_enable.val_i);
    cJSON_SetBoolValue(acl_mode.json, acl_mode.val_i);
    
    clearArray(acl_list.json);
    for(auto &entry : security_acl_entries) {
        cJSON_AddItemToArray(acl_list.json, cJSON_CreateString(entry.c_str()));
    }

    // ---------------------
    //         pools 
    // ---------------------
    
    clearArray(config_json.pools);
    for (size_t i = 0; i < pools_pools.size(); i++) {
        DHCPPool &p = pools_pools[i];

        cJSON *json_pool = cJSON_CreateObject();
        cJSON_AddStringToObject(json_pool, "name", p.name.c_str());
        cJSON_AddStringToObject(json_pool, "start", p.start_addr.c_str());
        cJSON_AddStringToObject(json_pool, "end", p.end_addr.c_str());
        cJSON_AddStringToObject(json_pool, "subnet", p.subnet.c_str());
        cJSON *pool_options = cJSON_AddArrayToObject(json_pool, "options");

        if (i + 1 < options_config_entries.size()) {
            options_config_entries[i + 1].json = pool_options;
            log(cJSON_Print(json_pool));
            cJSON_AddItemToArray(config_json.pools, json_pool);
        }
    }

    // --------------------
    //       options
    // --------------------
    apply_options(options_config_entries.front());
    for (size_t i = 1; i < options_config_entries.size(); i++) {
        apply_options(options_config_entries[i]);
    }

    // ---------------------
    //     write to file
    // ---------------------     
    // log(cJSON_Print(config_json.config));

    std::ofstream file(TabConfig::default_config_path, std::ios::binary); 
    char *new_settings = cJSON_Print(config_json.config);
    if (file.fail()) {
        error_msg = strerror(errno);
    } else if (!new_settings) {
        error_msg = "Error with configuration";
    } else {
        file << new_settings << std::endl;
        file.close();
    }
    return 0;
}

void TabConfig::options_load()
{
    options_list.clear();
    options_list.push_back("General");
    for(auto &entry : pools_entries) {
        options_list.push_back(entry);
    }

    if (options_list_selected != options_list_selected_last && options_list_selected >= 0) {
        options_loaded_list = options_config_entries.at(options_list_selected);

        options_loaded_list_entries.clear();
        for(auto &entry : options_loaded_list.options) {
            options_loaded_list_entries.push_back(std::to_string(entry.tag) + " - " +
                    DHCPv4_tag_to_full_name((DHCPv4_Options)entry.tag));
        }

        if (options_loaded_list.options.size() == 0) {
            options_loaded_list_entries.push_back("No options");
        }

        options_loaded_list_selected_last = -2;
        options_loaded_list_selected = 0;
        options_list_selected_last = options_list_selected;
    }

    if (options_loaded_list_selected != options_loaded_list_selected_last && options_loaded_list_selected >= 0) {
        // TODO: Fix crash when no options in list
        if (options_loaded_list.options.size() == 0) {
            options_loaded_type = "";
            options_value_container->DetachAllChildren();
            options_value_container->Add(Renderer([]{return text("No option selected");}));
            return;
        }
        switch(dhcp_option_tag_to_type((dhcp_option_type)options_loaded_list.options[options_loaded_list_selected].tag)) {
            case DHCP_OPTION_STRING: options_loaded_type = "string"; break;
            case DHCP_OPTION_BOOL: options_loaded_type = "boolean (0/1)"; break;
            case DHCP_OPTION_NUMERIC: options_loaded_type = "numeric"; break;
            case DHCP_OPTION_IP: options_loaded_type = "IP (example: 192.168.1.1)"; break;
            case DHCP_OPTION_BIN: options_loaded_type = "binary"; break;
        }

        options_value_container->DetachAllChildren();
        if (options_list_selected >= 0 && options_loaded_list_selected >= 0) {
            options_value_container->Add(Input(&this->options_config_entries[options_list_selected].
                             options[options_loaded_list_selected].value, "..."));
        }
        options_loaded_list_selected_last = options_loaded_list_selected;
    }
}

void TabConfig::dhcp_option_ctl(bool remove)
{
    if (dhcp_option_ctl_input.length() < 1 || std::stoi(dhcp_option_ctl_input) > 82)
        return;

    // Check if we have the needed 
    bool option_exists = false;
    int i = 0;
    for (auto &entry : options_loaded_list.options) {
        if (entry.tag == std::stoi(dhcp_option_ctl_input)) {
            option_exists = true;

            break;
        }
        i++;
    }

    // interface doesnt support binary configs
    if (dhcp_option_tag_to_type(std::stoi(dhcp_option_ctl_input)) == DHCP_OPTION_BIN) {
        return;
    }
    // remove if option exists
    if (remove && option_exists) {
        options_loaded_list_selected = 0;
        auto &vec = options_config_entries[options_list_selected].options;
        vec.erase(vec.begin() + i);

    } else if (!remove && !option_exists) {  
        // create if it doesnt exist
        DHCPOption o = {
            .tag = std::stoi(dhcp_option_ctl_input),
            .lenght = 0,
            .value = ""
        };
        options_config_entries[options_list_selected].options.push_back(o);
        options_loaded_list_selected = options_config_entries[options_list_selected].options.size() - 1;
    }

    options_list_selected_last = -1;
    options_load();
    options_loaded_list = options_config_entries[options_list_selected];
}

void TabConfig::pools_refresh()
{
    if (pools_pool_selected == pools_pool_selected_last)
        return;

    pools_entries.clear();
    for (auto &entry : pools_pools) {
        pools_entries.push_back(entry.name);
    }
    
    pools_value_container->DetachAllChildren();
    if (pools_entries.size() == 0) {
        pools_value_container->Add({
            Renderer([] {return text("No pool selected");})
        });
    } else {
        DHCPPool &p = pools_pools[pools_pool_selected];       

        pools_value_container->Add({Input(&p.name)});
        pools_value_container->Add({Input(&p.start_addr)});
        pools_value_container->Add({Input(&p.end_addr)});
        pools_value_container->Add({Input(&p.subnet)});
    }

    pools_pool_selected_last = pools_pool_selected;
}

void TabConfig::pool_ctl(bool remove)
{
    if (remove) {
        pools_pools.erase(pools_pools.begin() + pools_pool_selected);
        options_config_entries.erase(options_config_entries.begin() + pools_pool_selected + 1);
        pools_pool_selected = 0;
    } else {
        DHCPPool pool = {
            .name = "New pool",
            .start_addr = "0.0.0.0",
            .end_addr = "0.0.0.0",
            .subnet = "0.0.0.0",
            .options_json = cJSON_CreateArray()  
        };

        pools_pools.push_back(pool);

        DHCPOptionConfig option = {
            .json = pool.options_json
        };
        options_config_entries.push_back(option);

        pools_pool_selected = pools_pools.size() - 1;
    }

    pools_pool_selected_last = -1;
    options_list_selected = 0;
    options_list_selected_last = -1;
    pools_refresh();
    options_load();
}

