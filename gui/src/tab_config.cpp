#include "tab_config.hpp"
#include "cJSON.h"
#include "ftxui/component/component.hpp"
#include "ftxui/dom/elements.hpp"
#include "xtoy.hpp"
#include "logger.hpp"

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
// TabConfig::~TabConfig()
// {
    // cJSON_Delete(config_json.config);
// }

TabConfig::TabConfig()
{
    this->initialize();

    this->load_config_file();
    for(auto &entry : config_entries) {
        std::cout << entry.name << std::endl;
    }
    this->config_server_selected = 0;


    // We require the initialize function to be run before doing this.
    this->config_menu_server = Container::Horizontal({
        Container::Vertical({
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
        }), 
        // TODO: Try to make the description work
        // | CatchEvent([&] (Event event) {
        //     if ((event == Event::Character('j') || event == Event::ArrowDown) && this->config_server_selected / 2 < CONF_DB_ENABLE) {
        //         this->config_server_selected += 1;
        //     } else if ((event == Event::Character('k') || event == Event::ArrowUp) && this->config_server_selected / 2 > 0) {
        //         this->config_server_selected -= 1;
        //     }
        //     return false;
        // }),
        Renderer([] {return separatorEmpty();}),
        Renderer([&] {return paragraphAlignLeft(this->config_entries[this->config_server_selected].description);})
    });
    
    this->config_menu_pools = Renderer([] {return text("pools");});
    this->config_menu_options = Renderer([] {return text("options");});
    this->config_menu_security = Renderer([] {return text("security");});
    
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

}

static int tmp = 0;
void TabConfig::refresh()
{
    if (tmp < 1) {
        config_entries[5].val = "THIS IS A TEST VAL";
    } else {
        return;
    }
    tmp++;
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

    ConfEntry &entry = config_entries[0];
    for (int i = CONF_INTERFACE; i <= CONF_DB_ENABLE; i++) {
        entry = config_entries[i];
        log("LOADING " +entry.name + " " + entry.json_path);
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
                // entry.val.erase();
                if (!cJSON_IsNumber(entry.json)) {
                    log(entry.name + " Is not a number");
                    return -1;
                }
                entry.val = std::to_string(entry.json->valueint); //std::to_string(cJSON_GetNumberValue(entry.json));
                break;
            }
            case BOOLEAN: {
                if (!cJSON_IsBool(entry.json)) {
                    log(entry.name + " Is not a bool");
                    return -1;
                }
                entry.val_i = cJSON_GetNumberValue(entry.json);
                break;
            }
            default:
                break;
        }
        log(entry.name + " val end: " + entry.val);
    }

    return 0;
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
        .val_i = 1000,
        .def_val= "1000",
        .def_val_i = 1000,
    };
    config_entries.push_back(c_tick_delay);

    // Initialize 'Cache Size' entry.
    ConfEntry c_cache_size = {
        .name = "Cache Size",
        .description = "Maximum number of items stored in the server cache. The higher the size, the more clients can be handled at a time. Lower values provide protection against DHCP starvation attack.",
        .json_path = "cache_size",
        .type = NUMERIC,
        .val = "25",
        .val_i = 25,
        .def_val= "25",
        .def_val_i = 25,
    };
    config_entries.push_back(c_cache_size);

    // Initialize 'Transaction Duration' entry.
    ConfEntry c_transaction_duration = {
        .name = "Transaction Duration",
        .description = "Duration (in seconds) of a DHCP transaction stored in cache. Similiar to the cache size, higher values will provide protection against DHCP starvation attacks by making that transaction last in cache for longer.",
        .json_path = "transaction_duration",
        .type = NUMERIC,
        .val = "60",
        .val_i = 60,
        .def_val = "60",
        .def_val_i = 60,
    };
    config_entries.push_back(c_transaction_duration);

    // Initialize 'Lease Expiration Check' entry.
    ConfEntry c_lease_expiration_check = {
        .name = "Lease Expiration Check",
        .description = "Interval (in seconds) between each lease expiration check. Server periodically checks for expired DHCP leases. Lower values mean that expired addresses are returned to pool sooner.",
        .json_path = "lease_expiration_check",
        .type = NUMERIC,
        .val = "60",
        .val_i = 60,
        .def_val = "60",
        .def_val_i = 60,
    };
    config_entries.push_back(c_lease_expiration_check);

    // Initialize 'Log Verbosity' entry.
    ConfEntry c_log_verbosity = {
        .name = "Log Verbosity",
        .description = "Level of verbosity for server logging. 1 - Only critical errors, 2 - All errors, 3 - Warnings, 4 (default) messages note taking look at, 5 - very verbose, all messages present.",
        .json_path = "log_verbosity",
        .type = NUMERIC,
        .val = "4",
        .val_i = 4,
        .def_val= "4",
        .def_val_i = 4,
    };
    config_entries.push_back(c_log_verbosity);

    // Initialize 'Lease Time' entry.
    ConfEntry c_lease_time = {
        .name = "Lease Time",
        .description = "Duration (in seconds) of a DHCP lease. This is a global setting for lease duration. WARNING: setting this option in options section of config will have NO EFFECT.",
        .json_path = "lease_time",
        .type = NUMERIC,
        .val = "43200",
        .val_i = 43200,
        .def_val = "43200",
        .def_val_i = 43200,
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
    
    log(cJSON_Print(config_json.config));
    // TODO: DONT FORGET to actually overwrite the config file

    return 0;
}

