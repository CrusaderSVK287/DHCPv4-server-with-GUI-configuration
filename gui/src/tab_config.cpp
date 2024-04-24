#include "tab_config.hpp"
#include "cJSON.h"
#include "ftxui/component/component.hpp"
#include "ftxui/dom/elements.hpp"
#include "xtoy.hpp"
#include "logger.hpp"

#include <climits>
#include <exception>
#include <fstream>
#include <iostream>
#include <ostream>
#include <stdexcept>
#include <string>

using namespace ftxui;

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
    
    this->config_menu_pools = Renderer([] {return text("pools");});
    this->config_menu_options = Renderer([] {return text("options");});

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

    this->tab_contents = Container::Horizontal({
        Container::Vertical({
            Menu(&TabConfig::config_menu_entries, &this->config_menu_selected) | size(WIDTH, EQUAL, 11),
            Button("Apply", [&] {this->apply_settings();}),
        }),
        Renderer([]{return separator();}),
        this->config_menu_tab,
    });

    this->security_acl_entries_size_last = INT_MAX;
}

void TabConfig::refresh()
{
    update_acl_entries();
}

int TabConfig::load_config_file()
{
    std::ifstream file(TabConfig::default_config_path, std::ios::binary);
    if (!file) {
        return -1;
    }

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
        acl_enable.json = cJSON_AddArrayToObject(config_json.security, acl_list.json_path.c_str());
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

    return 0;
}

void TabConfig::update_acl_entries()
{
    if (security_acl_entries.size() == security_acl_entries_size_last)
        return;

    config_menu_security_acl_entries->DetachAllChildren();
    // config_menu_security_acl_entries.
    for(auto &entry : security_acl_entries) {
        log(entry);
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
        .json_path = "entries",
        .type = STRING,
        .val = "",
        .def_val = "",
    };
    config_entries.push_back(c_acl_entries);

    if (config_entries.size() != CONFIG_COUNT)
        throw std::runtime_error("TabConfig inproperly initialized");

    return 0;
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


    log(cJSON_Print(config_json.config));
    // TODO: DONT FORGET to actually overwrite the config file

    return 0;
}

