
#ifndef __DHCPRELEASE_H__
#define __DHCPRELEASE_H__

#include "../dhcp_server.h"
#include "../dhcp_packet.h"

/* Handle DHCPRELEASE messages */
int message_dhcprelease_handle(dhcp_server_t *server, dhcp_message_t *message);

#endif // !__DHCPRELEASE_H__

