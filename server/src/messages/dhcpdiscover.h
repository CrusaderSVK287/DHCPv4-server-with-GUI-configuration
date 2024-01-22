#ifndef __DHCPDISCOVER_H__
#define __DHCPDISCOVER_H__

#include "../dhcp_server.h"
#include "../dhcp_packet.h"

/* Handle DHCPDISCOVER messages */
int message_dhcpdiscover_handle(dhcp_server_t *server, dhcp_message_t *message);

#endif // !__DHCPDISCOVER_H__

