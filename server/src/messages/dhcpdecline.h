
#ifndef __DHCPDECLINE_H__
#define __DHCPDECLINE_H__

#include "../dhcp_server.h"
#include "../dhcp_packet.h"

/* Handle DHCPDECLINE messages */
int message_dhcpdecline_handle(dhcp_server_t *server, dhcp_message_t *message);

#endif // !__DHCPDECLINE_H__

