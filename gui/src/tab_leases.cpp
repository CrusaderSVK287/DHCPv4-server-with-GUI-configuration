#include "tab_leases.hpp"
#include "xtoy.hpp"
#include <cstring>
#include <filesystem>
#include <fstream>
#include <ftxui/component/component.hpp>
#include <ftxui/dom/elements.hpp>

#include <cJSON.h>
#include <cJSON_Utils.h>
#include <iostream>
#include <iomanip>
#include <sstream>

using namespace ftxui;
namespace fs = std::filesystem;

TabLease::TabLease()
{
    this->pool_selected = 0;
    this->lease_selected = 0;
    
    this->lease_details_pool_name = "";
    this->lease_details_address = "";
    this->lease_details_subnet = "";
    this->lease_details_xid = "";
    this->lease_details_start = "";
    this->lease_details_end = "";
    this->lease_details_mac = "";
    
    this->tab_contents = Container::Horizontal({
        Container::Vertical({
            Renderer([] {
                return vbox({
                    text(" Pools ") ,
                    separator()
                });
            }) ,
            Menu({&this->pools, &this->pool_selected}),
        }) | flex | size(WIDTH, LESS_THAN, 27) | size(WIDTH, GREATER_THAN, 7),
        Renderer([] {return separator();}),

        Container::Vertical({
            Renderer([] {
                return vbox({
                    text(" leased addresses "),
                    separator()
                });
            }),
            Menu({&this->leased_addresses, &this->lease_selected}),
        }) | flex | size(WIDTH, EQUAL, 18),
        Renderer([] {return separator();}),

        Container::Vertical({
            Renderer([&] { 
                return vbox({
                    text(" lease details "), 
                    separator(),

                    hbox({text("Pool:    ") | bold, text(this->lease_details_pool_name)}),
                    hbox({text("Address: ") | bold, text(this->lease_details_address)}),
                    hbox({text("Subnet:  ") | bold, text(this->lease_details_subnet)}),
                    hbox({text("xid:     ") | bold, text(this->lease_details_xid)}),
                    hbox({text("Leased:  ") | bold, text(this->lease_details_start)}),
                    hbox({text("Expires: ") | bold, text(this->lease_details_end)}),
                    hbox({text("Client:  ") | bold, text(this->lease_details_mac)}),
                });
            }),

        }) | flex,
    });
}

void TabLease::refresh()
{
    load_pools();
    load_leases();
    load_lease_details();
}

void TabLease::load_pools()
{
    pools.clear();
    size_t last_dot = 0;
    size_t len_of_prefix = std::strlen(TabLease::leases_path);

    for (auto &entry : fs::directory_iterator(TabLease::leases_path)) {
        last_dot = entry.path().string().find_last_of('.');
        pools.push_back(entry.path().string().substr(len_of_prefix, last_dot - len_of_prefix) + " ");
    }
}

int TabLease::parse_json_leases(cJSON *_json, std::string _pool_name)
{
    if (!_json) 
        return -1;

    leases.clear();
    leased_addresses.clear();
    /* Buffer to store lease data */
    Lease lease = {0};
    lease.pool_name = _pool_name;

    cJSON *json = cJSON_GetObjectItem(_json, "leases");
    if (!json || !cJSON_IsArray(json))
        return -1;

    cJSON *element = NULL;
    cJSON_ArrayForEach(element, json) {
        lease.address = ipv4_address_to_uint32(
                        cJSON_GetStringValue(
                        cJSON_GetObjectItem(element, "address")));
        lease.subnet  = ipv4_address_to_uint32(
                        cJSON_GetStringValue(
                        cJSON_GetObjectItem(element, "subnet")));

        lease.lease_start  = cJSON_GetNumberValue(cJSON_GetObjectItem(element, "lease_start" ));
        lease.lease_expire = cJSON_GetNumberValue(cJSON_GetObjectItem(element, "lease_expire"));
        lease.xid          = cJSON_GetNumberValue(cJSON_GetObjectItem(element, "xid"         ));
        lease.flags        = cJSON_GetNumberValue(cJSON_GetObjectItem(element, "flags"       ));

        mac_to_uint8_array(cJSON_GetStringValue(cJSON_GetObjectItem(element, "client_mac_address")),
                lease.client_mac_address);

        leases.push_back(lease);
        leased_addresses.push_back(cJSON_GetStringValue(cJSON_GetObjectItem(element, "address")));
    }

    return 0;
}

int TabLease::load_leases() {

    std::string pool = pools[pool_selected];
    std::string path = TabLease::leases_path + pool.substr(0, pool.size() - 1) + ".lease";

    std::ifstream file(path, std::ios::binary);
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

    cJSON *json_leases = cJSON_Parse(buffer.get());
    if (!json_leases) {
        return -1;
    }

    if (parse_json_leases(json_leases, pool)) {
        return -1;
    }

    return 0;
}

void TabLease::load_lease_details()
{
    /* If no leases, just return */
    if (leases.size() == 0) {
        lease_details_pool_name = "";
        lease_details_address = "";
        lease_details_subnet = "";
        lease_details_xid = "";
        lease_details_start = "";
        lease_details_end = "";
        lease_details_mac = "";
        return;
    }

    Lease &lease = leases[lease_selected]; 
    
    lease_details_pool_name = lease.pool_name;
    {
        std::stringstream ss; ss << uint32_to_ipv4_address(lease.address);
        lease_details_address = ss.str();
    }
    {
        std::stringstream ss; ss << uint32_to_ipv4_address(lease.subnet);
        lease_details_subnet = ss.str();
    }
    {
        std::stringstream ss; ss << "0x" << std::setw(8) << std::setfill('0') << std::hex << lease.xid;
        lease_details_xid = ss.str();
    }
    {
        std::time_t time = lease.lease_start;
        std::tm* t = std::gmtime(&time);
        std::stringstream ss; ss << std::put_time(t, "%Y-%m-%d %I:%M:%S %p");
        lease_details_start = ss.str();
    }
    {
        std::time_t time = lease.lease_expire;
        std::tm *t = std::gmtime(&time);
        std::stringstream ss; ss << std::put_time(t, "%Y-%m-%d %I:%M:%S %p");
        lease_details_end = ss.str();
    }
    {
        std::stringstream ss; ss << uint8_array_to_mac(lease.client_mac_address);
        lease_details_mac = ss.str();
    }
}

