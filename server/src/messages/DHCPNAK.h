
#ifndef __DHCPNAK_H__
#define __DHCPNAK_H__

#include "../dhcp_server.h"
#include "../dhcp_packet.h"

/* Handle DHCPNAK messages */
int message_DHCPNAK_send(dhcp_server_t *server, dhcp_message_t *message);

#endif // !__DHCPNAK_H__

