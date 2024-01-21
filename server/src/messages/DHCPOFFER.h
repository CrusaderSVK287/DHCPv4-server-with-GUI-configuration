
#ifndef __DHCPOFFER_H__
#define __DHCPOFFER_H__

#include "../dhcp_server.h"
#include "../dhcp_packet.h"

/* Handle DHCPOFFER messages */
int message_DHCPOFFER_send(dhcp_server_t *server, dhcp_message_t *message);

#endif // !__DHCPOFFER_H__

