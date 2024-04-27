#include "tab_command.hpp"
#include <cJSON.h>
#include <cJSON_Utils.h>
#include <cstdio>
#include <cstring>
#include <iterator>
#include <sstream>
#include <ftxui/component/component.hpp>
#include <ftxui/dom/elements.hpp>
#include <string>
#include "logger.hpp"

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#define SOCKET_PATH "/run/dhcp.sock"

using namespace ftxui;

static cJSON *json_error(std::string error)
{
    cJSON *json = cJSON_CreateArray();
    cJSON_AddItemToArray(json, cJSON_CreateString(error.c_str()));

    return json;
}

TabCommand::TabCommand()
{
    output_selected = 0;

    tab_contents = Container::Vertical({
        Container::Horizontal({
            Input(&this->input, "Enter your command here.."),
        }) | border | CatchEvent([&] (Event event) {
            // If the event is NOT enter, exit
            if (event != Event::Return) {
                return false;
            } 
            if (this->input.length() == 0) {
                return true;
            }


            handle_command();
            this->input.clear();
            this->output_selected = this->output.size() - 1;

            return true;
        }),
        Menu(&this->output, &this->output_selected) | vscroll_indicator | yframe | yflex | border,
    });

    initialize_commands();
}

void TabCommand::initialize_commands()
{
    // Localy handled commands
    this->commands.push_back({"clear", false, &TabCommand::command_clear, "Deletes all output"});
    this->commands.push_back({"help", false, &TabCommand::command_help, "Shows short help page for each command"});

    // Server handled commands
    this->commands.push_back({"echo", true, nullptr, "Echoes back what the user types in arguments"});
}

void TabCommand::refresh()
{

}

void TabCommand::handle_command()
{
    size_t space_pos = input.find(' ');
        std::string sub;
    if (space_pos != std::string::npos) {
        sub = input.substr(0, space_pos);
    } else {
        // If no space is found, extract the whole string
        sub = input;
    }

    for (auto &entry : commands) {
        if (entry.name.compare(sub) != 0) {
            continue;
        }

        output.push_back("$ " + input);
        if (entry.ipc) {
            print_response(send_command());
        } else {
            (this->*(entry.func))();
        }

        return;
    }

    output.push_back("Unknown command: " + input);
}

static char* json_create_command(std::string input)
{
    cJSON *json = cJSON_CreateObject();
    if (!json) {
        return NULL;
    }
    std::istringstream iss(input);
    std::vector<std::string> words {
        std::istream_iterator<std::string>{iss},
        std::istream_iterator<std::string>{}
    };
    cJSON_AddStringToObject(json, "command", words.front().c_str());
    cJSON *array = cJSON_AddArrayToObject(json, "parameters");
    for (size_t i = 1; i < words.size(); i++) {
        cJSON_AddItemToArray(array, cJSON_CreateString(words[i].c_str()));
    }

    char *json_string = cJSON_PrintUnformatted(json);
    cJSON_Delete(json);
    if (!json_string) {
        return NULL;
    }

    return json_string;
}

cJSON *TabCommand::send_command()
{
    int client_socket;
    struct sockaddr_un server_address;

    // Create a socket
    client_socket = socket(AF_UNIX, SOCK_STREAM, 0);
    if (client_socket == -1) {
        return json_error("Failed to create socket");
    }

    // Initialize server address structure
    server_address.sun_family = AF_UNIX;
    strncpy(server_address.sun_path, SOCKET_PATH, sizeof(server_address.sun_path) - 1);

    // Connect to the server
    if (connect(client_socket, (struct sockaddr *)&server_address, sizeof(server_address)) == -1) {
        return json_error("Connection failed");
    }

    // construct a json from the input in this format:
    // {
    //   "command": "command_name",
    //   "parameters": [
    //     "parameter1",
    //     "parameter2"
    //   ]
    // }
    char *json_string = json_create_command(input);
    if (!json_string) {
        return json_error("Failed to create a command");
    }

    // Send the message to the server
    if (send(client_socket, json_string, strlen(json_string), 0) == -1) {
        return json_error("Send failed");
    }
    free(json_string);

    // Receive response from server
    char buffer[BUFSIZ];
    memset(buffer, 0, BUFSIZ);
    int bytes_received = recv(client_socket, buffer, BUFSIZ, 0);
    if (bytes_received <= 0) {
        return json_error("Receive failed or connection closed");
    }

    close(client_socket);

    return cJSON_Parse(buffer);
}

void TabCommand::print_response(cJSON *json)
{
    if (!json) {
        output.push_back("ERROR: Invalid JSON retuned");
        return;
    }

    if (!cJSON_IsArray(json)) {
        output.push_back("ERROR: returned value is not a JSON array");
        return;
    }

    cJSON *e;
    cJSON_ArrayForEach(e, json) {
        if (!cJSON_IsString(e)) {
            output.push_back("ERROR: This value is not a string");
        } else {
            output.push_back(cJSON_GetStringValue(e));
        }
    }
}

