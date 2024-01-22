
#ifndef __DHCPOFFER_H__
#define __DHCPOFFER_H__

#include "../dhcp_server.h"
#include "../dhcp_packet.h"
#include <stdint.h>

/* Handle DHCPOFFER messages */
int message_dhcpoffer_send(dhcp_server_t *server, dhcp_message_t *message);

/* Build DHCPOFFER message */
int message_dhcpoffer_build(dhcp_server_t *server, dhcp_message_t *message, 
        uint32_t offered_address, uint32_t offered_lease_duration);

#endif // !__DHCPOFFER_H__

