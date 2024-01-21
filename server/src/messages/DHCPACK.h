
#ifndef __DHCPACK_H__
#define __DHCPACK_H__

#include "../dhcp_server.h"
#include "../dhcp_packet.h"

/* Handle DHCPACK messages */
int message_DHCPACK_send(dhcp_server_t *server, dhcp_message_t *message);

#endif // !__DHCPACK_H__

