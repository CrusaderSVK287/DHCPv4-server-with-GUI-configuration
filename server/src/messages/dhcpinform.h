
#ifndef __DHCPINFORM_H__
#define __DHCPINFORM_H__

#include "../dhcp_server.h"
#include "../dhcp_packet.h"

/* Handle DHCPINFORM messages */
int message_dhcpinform_handle(dhcp_server_t *server, dhcp_message_t *message);

#endif // !__DHCPINFORM_H__

