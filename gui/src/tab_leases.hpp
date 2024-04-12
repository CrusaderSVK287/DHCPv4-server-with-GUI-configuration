#ifndef __TAB_LEASES_HPP__
#define __TAB_LEASES_HPP__

#include <ftxui/component/component_base.hpp>
#include <cJSON.h>
#include "tab.hpp"

struct Lease {
    uint32_t address;
    uint32_t subnet;
    std::string pool_name;

    uint32_t lease_start;
    uint32_t lease_expire;

    uint32_t xid;
    uint8_t flags;
    uint8_t client_mac_address[6];
};

class TabLease : public UITab {
public:
    TabLease();
    void refresh();
private:
    static constexpr const char* leases_path = "/etc/dhcp/lease/";

    /* The lease struct holds this in not so favorable way */
    std::string lease_details_pool_name;
    std::string lease_details_address;
    std::string lease_details_subnet;
    std::string lease_details_xid;
    std::string lease_details_start;
    std::string lease_details_end;
    std::string lease_details_mac;

    int pool_selected;
    int lease_selected;
    std::vector<std::string> pools;
    std::vector<std::string> leased_addresses;
    std::vector<Lease> leases;

    void load_pools();
    int load_leases();
    void load_lease_details();
    int parse_json_leases(cJSON *json, std::string _pool_name);
};

#endif // __TAB_LEASES_HPP__

