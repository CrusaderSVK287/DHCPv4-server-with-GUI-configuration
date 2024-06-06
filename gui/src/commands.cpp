#include "tab_command.hpp"
#include "tab_config.hpp"

#include <cJSON.h>
#include <cerrno>
#include <cstring>
#include <fstream>
#include <iostream>
#include <ostream>
#include <random>
#include <stdexcept>
#include <string>
#include "logger.hpp"
#include <iostream>
#include <sstream>
#include <vector>
#include <cerrno> // For errno
#include <cstring> // For strerrorinclude <cstdio>


void TabCommand::command_clear()
{
    output.clear();
    output_selected = 0;
}

void TabCommand::command_help()
{
    output.push_back("Short description of each command");

    for(auto &entry : commands) {
        if (entry.name.compare("?") != 0) {
            output.push_back("  " + entry.name + " : " + entry.help);
            output.push_back("    Usage: " + entry.usage);
        }
    }
}

bool is_number(const std::string& s)
{
    std::string::const_iterator it = s.begin();
    while (it != s.end() && std::isdigit(*it)) ++it;
    return !s.empty() && it == s.end();
}

static cJSON* config_get_path(cJSON *root, std::string path)
{
    std::stringstream ss(path);
    std::string word;

    cJSON *result = root;

    while (std::getline(ss, word, '.')){
        if (is_number(word)) {
            result = cJSON_GetArrayItem(result, std::stoi(word));
        } else {
            result = cJSON_GetObjectItem(result, word.c_str());
        }
    }

    return result;
}

void TabCommand::command_config()
{
    cJSON *json;
    std::ifstream file(TabConfig::default_config_path, std::ios::binary);
    if (!file) {
        output.push_back("Error opening config file");
        return;
    }

    file.seekg(0, std::ios::end);
    std::streampos file_size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::unique_ptr<char[]> buffer(new char[file_size]);
    if (!buffer) {
        file.close();
        output.push_back("Error opening config file");
        return;
    }

    file.read(buffer.get(), file_size);

    json = cJSON_Parse(buffer.get());
    if (!json) {
        output.push_back("Erorr: failed to parse JSON");
        return;
    }
    file.close();

    // Split the input string into substrings
    std::stringstream ss(input);
    std::string word;
    std::vector<std::string> substrings;

    while (std::getline(ss, word, ' ') && substrings.size() < 4) {
        substrings.push_back(word);
    }

    // Check if we have exactly 4 substrings
    if (substrings.size() < 3) {
        output.push_back("Error: Bad input format, check usage");
        return;
    }

    // retrieve config. 3rd substring is path
    cJSON *conf = config_get_path(json, substrings[2]);
    if (!conf) {
        output.push_back("Error: Could not find config entry " + substrings[2]);
        return;
    }

    // print value
    if (substrings[1].compare("get") == 0) {
        switch (conf->type) {
            case cJSON_String: output.push_back(conf->valuestring); break;
            case cJSON_Number: output.push_back(std::to_string(conf->valueint)); break;
            case cJSON_False:  output.push_back("False"); break;
            case cJSON_True:   output.push_back("True"); break;
            default:
                output.push_back("Error: Cannot show value of this object");
                break;
        }
    }

    if (substrings[1].compare("set") == 0) {
        if (substrings.size() != 4) {
            output.push_back("Error: Missing new value, check command usage");
            goto end_of_set;
        }

        switch (conf->type) {
            case cJSON_String: cJSON_SetValuestring(conf, substrings[3].c_str()); break;
            case cJSON_Number: {
                if (!is_number(substrings[3])) {
                    output.push_back("Error: expected number");
                    break;
                }
                cJSON_SetNumberValue(conf, std::stoi(substrings[3])); 
                break;
            }
            case cJSON_True:   
            case cJSON_False:  {
                if (substrings[3][0] != '0' && substrings[3][0] != '1') {
                    output.push_back("Error: expected boolean (0|1)");
                    break;
                }
                cJSON_SetBoolValue(conf, substrings[3][0] - '0'); 
                break;
            }
            default:
                output.push_back("Error: Cannot set value of this object");
                goto end_of_set;
        }

        // write to file 
        
        std::ofstream file(TabConfig::default_config_path, std::ios::binary); 
        char *new_settings = cJSON_Print(json);
        if (file.fail()) {
            output.push_back("Failed to open config file: " + std::to_string(errno));
        } else if (!new_settings) {
            output.push_back("Failed to generate new JSON string");
        } else {
            file << new_settings << std::endl;
            file.close();
            free(new_settings);
        }
    }
end_of_set:

    cJSON_Delete(json);
}

