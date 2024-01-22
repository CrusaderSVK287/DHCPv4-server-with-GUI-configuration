
#ifndef __DHCPACK_H__
#define __DHCPACK_H__

#include "../dhcp_server.h"
#include "../dhcp_packet.h"

/* Handle DHCPACK messages */
int message_dhcpack_send(dhcp_server_t *server, dhcp_message_t *message);

int message_dhcpack_build(dhcp_server_t *server, dhcp_message_t *dhcp_request, 
    uint32_t offered_lease_duration);
#endif // !__DHCPACK_H__

