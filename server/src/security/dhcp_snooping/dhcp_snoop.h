#ifndef __DHCP_SNOOP_H__
#define __DHCP_SNOOP_H__

#include "../../dhcp_server.h"
#include <limits.h>

#ifdef CONFIG_SECURITY_ENABLE_DHCP_SNOOPING
#undef CONFIG_SECURITY_ENABLE_DHCP_SNOOPING
#endif
#ifdef _CONFIG_SECURITY_ENABLE_DHCP_SNOOPING
#define CONFIG_SECURITY_ENABLE_DHCP_SNOOPING
#endif

enum dhcp_snooper_status {
    DHCP_SNOOP_DISABLED = INT_MIN,

    DHCP_SNOOP_ERROR = -1,
    DHCP_SNOOP_NO_THREAT = 0,
    DHCP_SNOOP_POTENTIAL_ROGUE = 1,
};

enum dhcp_snooper_status dhcp_snooper_perform_scan(dhcp_server_t *server, 
                                                   const char *spoofed_mac,
                                                   llist_t *server_whitelist,
                                                   char **status_msg);

/* Simple function to check xid of a message whether it matches a fake DHCPDISCOVER sent by the scan */
int dhcp_snooper_check_xid(uint32_t xid);

#endif // !__DHCP_SNOOP_H__

