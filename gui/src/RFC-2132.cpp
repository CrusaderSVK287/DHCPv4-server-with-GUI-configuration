#include "RFC-2132.hpp"

std::string DHCPv4_tag_to_full_name(enum DHCPv4_Options tag)
{
    switch (tag) {
    case DHCP_OPTION_PAD:                       return "Pad";
    case DHCP_OPTION_SUBNET_MASK:               return "Subnet mask";
    case DHCP_OPTION_TIME_OFFSET:               return "Time offset";
    case DHCP_OPTION_ROUTER:                    return "Router";
    case DHCP_OPTION_TIME_SERVERS:              return "Time servers";
    case DHCP_OPTION_NAME_SERVERS:              return "Name servers";
    case DHCP_OPTION_DOMAIN_NAME_SERVERS:       return "Domain name servers";
    case DHCP_OPTION_LOG_SERVERS:               return "Log servers";
    case DHCP_OPTION_COOKIE_SERVERS:            return "Cookie servers";
    case DHCP_OPTION_LPR_SERVERS:               return "LPR servers";
    case DHCP_OPTION_IMPRESS_SERVERS:           return "Impress servers";
    case DHCP_OPTION_RESOURCE_LOCATION_SERVERS: return "Resource location servers";
    case DHCP_OPTION_HOST_NAME:                 return "Host name";
    case DHCP_OPTION_BOOT_FILE_SIZE:            return "Boot file size";
    case DHCP_OPTION_MERIT_DUMP:                return "Merit dump";
    case DHCP_OPTION_DOMAIN_NAME:               return "Domain name";
    case DHCP_OPTION_SWAP_SERVER:               return "Swap server";
    case DHCP_OPTION_ROOT_PATH:                 return "Root path";
    case DHCP_OPTION_EXTENSIONS_PATH:           return "Extensions path";
    case DHCP_OPTION_IP_FORWARDING_ENABLE_DISABLE: 
                                                return "IP forwarding enable/disable";
    case DHCP_OPTION_NON_LOCAL_SOURCE_ROUTING_ENABLE_DISABLE: 
                                                return "Non-local source routing enable/disable";
    case DHCP_OPTION_POLICY_FILTER:             return "Policy filter";
    case DHCP_OPTION_MAX_DGRAM_REASSEMBLY_SIZE: return "Max datagram reassembly size";
    case DHCP_OPTION_DEFAULT_IP_TTL:            return "Default IP TTL";
    case DHCP_OPTION_PATH_MTU_AGING_TIMEOUT:    return "Path MTU aging timeout";
    case DHCP_OPTION_PATH_MTU_PLATEAU_TABLE:    return "Path MTU plateau table";
    case DHCP_OPTION_INTERFACE_MTU:             return "Interface MTU";
    case DHCP_OPTION_ALL_SUBNETS_LOCAL:         return "All subnets local";
    case DHCP_OPTION_BROADCAST_ADDRESS:         return "Broadcast address";
    case DHCP_OPTION_PERFORM_MASK_DISCOVERY:    return "Perform mask discovery";
    case DHCP_OPTION_MASK_SUPPLIER:             return "Mask supplier";
    case DHCP_OPTION_ROUTER_DISCOVERY:          return "Router discovery";
    case DHCP_OPTION_ROUTER_SOLICITATION_ADDRESS: 
                                                return "Router solicitation address";
    case DHCP_OPTION_STATIC_ROUTE:              return "Static route";
    case DHCP_OPTION_TRAILER_ENCAPSULATION:     return "Trailer encapsulation";
    case DHCP_OPTION_ARP_CACHE_TIMEOUT:         return "ARP cache timeout";
    case DHCP_OPTION_IEEE802_3_ENCAPSULATION:   return "IEEE802.3 encapsulation";
    case DHCP_OPTION_DEFAULT_TCP_TTL:           return "Default TCP TTL";
    case DHCP_OPTION_TCP_KEEPALIVE_INTERVAL:    return "TCP keepalive interval";
    case DHCP_OPTION_TCP_KEEPALIVE_GARBAGE:     return "TCP keepalive garbage";
    case DHCP_OPTION_NIS_DOMAIN:                return "NIS domain";
    case DHCP_OPTION_NIS_SERVERS:               return "NIS servers";
    case DHCP_OPTION_NTP_SERVERS:               return "NTP servers";
    case DHCP_OPTION_VENDOR_SPECIFIC_INFO:      return "Vendor specific info";
    case DHCP_OPTION_NETBIOS_NAME_SERVERS:      return "NetBIOS name servers";
    case DHCP_OPTION_NETBIOS_DD_SERVER:         return "NetBIOS DD server";
    case DHCP_OPTION_NETBIOS_NODE_TYPE:         return "NetBIOS node type";
    case DHCP_OPTION_NETBIOS_SCOPE:             return "NetBIOS scope";
    case DHCP_OPTION_X_WINDOW_FONT_SERVER:      return "X window font server";
    case DHCP_OPTION_X_WINDOW_DISPLAY_MANAGER:  return "X window display manager";
    case DHCP_OPTION_REQUESTED_IP_ADDRESS:      return "Requested IP address";
    case DHCP_OPTION_IP_ADDRESS_LEASE_TIME:     return "IP address lease time";
    case DHCP_OPTION_OVERLOAD:                  return "Overload";
    case DHCP_OPTION_DHCP_MESSAGE_TYPE:         return "DHCP message type";
    case DHCP_OPTION_SERVER_IDENTIFIER:         return "Server identifier";
    case DHCP_OPTION_PARAMETER_REQUEST_LIST:    return "Parameter request list";
    case DHCP_OPTION_MESSAGE:                   return "Message";
    case DHCP_OPTION_MAX_DHCP_MESSAGE_SIZE:     return "Max DHCP message size";
    case DHCP_OPTION_RENEWAL_TIME_VALUE:        return "Renewal time value";
    case DHCP_OPTION_REBINDING_TIME_VALUE:      return "Rebinding time value";
    case DHCP_OPTION_VENDOR_CLASS_IDENTIFIER:   return "Vendor class identifier";
    case DHCP_OPTION_CLIENT_IDENTIFIER:         return "Client identifier";
    case DHCP_OPTION_NWIP_DOMAIN_NAME:          return "NWIP domain name";
    case DHCP_OPTION_NWIP_SUBOPTIONS:           return "NWIP suboptions";
    case DHCP_OPTION_USER_CLASS:                return "User class";
    case DHCP_OPTION_FQDN:                      return "FQDN";
    case DHCP_OPTION_DHCP_AGENT_OPTIONS:        return "DHCP agent options";
    case DHCP_OPTION_END:                       return "End";
    default: return "Unknown";
    }
}

// TODO: Finish the switch, only example / common are here now
enum dhcp_option_type dhcp_option_tag_to_type(int tag) {
    switch(tag) {
    case DHCP_OPTION_SUBNET_MASK:
    case DHCP_OPTION_ROUTER:
    case DHCP_OPTION_TIME_SERVERS:
    case DHCP_OPTION_NAME_SERVERS:
    case DHCP_OPTION_DOMAIN_NAME_SERVERS:
    case DHCP_OPTION_LOG_SERVERS:
    case DHCP_OPTION_COOKIE_SERVERS:
    case DHCP_OPTION_LPR_SERVERS:
    case DHCP_OPTION_SERVER_IDENTIFIER:
    case DHCP_OPTION_IMPRESS_SERVERS:
    case DHCP_OPTION_RESOURCE_LOCATION_SERVERS:
    case DHCP_OPTION_SWAP_SERVER:
    case DHCP_OPTION_BROADCAST_ADDRESS:
    case DHCP_OPTION_ROUTER_SOLICITATION_ADDRESS:
    case DHCP_OPTION_STATIC_ROUTE:
    case DHCP_OPTION_ARP_CACHE_TIMEOUT:
    case DHCP_OPTION_NIS_SERVERS:
    case DHCP_OPTION_NTP_SERVERS:
    case DHCP_OPTION_NETBIOS_NAME_SERVERS:
    case DHCP_OPTION_NETBIOS_DD_SERVER:
    case DHCP_OPTION_X_WINDOW_FONT_SERVER:
    case DHCP_OPTION_X_WINDOW_DISPLAY_MANAGER:
    case DHCP_OPTION_REQUESTED_IP_ADDRESS:
        return DHCP_OPTION_IP;

    case DHCP_OPTION_TIME_OFFSET:
    case DHCP_OPTION_BOOT_FILE_SIZE:
    case DHCP_OPTION_DEFAULT_IP_TTL:
    case DHCP_OPTION_PATH_MTU_AGING_TIMEOUT:
    case DHCP_OPTION_INTERFACE_MTU:
    case DHCP_OPTION_MAX_DGRAM_REASSEMBLY_SIZE:
    case DHCP_OPTION_DEFAULT_TCP_TTL:
    case DHCP_OPTION_TCP_KEEPALIVE_INTERVAL:
    case DHCP_OPTION_TCP_KEEPALIVE_GARBAGE:
    case DHCP_OPTION_IP_ADDRESS_LEASE_TIME:
    case DHCP_OPTION_NETBIOS_NODE_TYPE:
    case DHCP_OPTION_RENEWAL_TIME_VALUE:
    case DHCP_OPTION_REBINDING_TIME_VALUE:
    case DHCP_OPTION_DHCP_MESSAGE_TYPE:
        return DHCP_OPTION_NUMERIC;

    case DHCP_OPTION_IP_FORWARDING_ENABLE_DISABLE:
    case DHCP_OPTION_NON_LOCAL_SOURCE_ROUTING_ENABLE_DISABLE:
    case DHCP_OPTION_ROUTER_DISCOVERY:
    case DHCP_OPTION_ALL_SUBNETS_LOCAL:
    case DHCP_OPTION_MASK_SUPPLIER:
    case DHCP_OPTION_TRAILER_ENCAPSULATION:
    case DHCP_OPTION_PERFORM_MASK_DISCOVERY:
        return DHCP_OPTION_BOOL;

    case DHCP_OPTION_POLICY_FILTER:
    case DHCP_OPTION_HOST_NAME:
    case DHCP_OPTION_NIS_DOMAIN:
    case DHCP_OPTION_DOMAIN_NAME:
    case DHCP_OPTION_USER_CLASS:
    case DHCP_OPTION_FQDN:
    case DHCP_OPTION_ROOT_PATH:
    case DHCP_OPTION_EXTENSIONS_PATH:
        return DHCP_OPTION_STRING;

    default:
        return DHCP_OPTION_BIN;
    }
}
