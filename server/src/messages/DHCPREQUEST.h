
#ifndef __DHCPREQUEST_H__
#define __DHCPREQUEST_H__

#include "../dhcp_server.h"
#include "../dhcp_packet.h"

/* Handle DHCPREQUEST messages */
int message_DHCPREQUEST_handle(dhcp_server_t *server, dhcp_message_t *message);

#endif // !__DHCPREQUEST_H__

