
#ifndef __DHCPACK_H__
#define __DHCPACK_H__

#include "../dhcp_server.h"
#include "../dhcp_packet.h"

/* Handle DHCPACK messages */
/*
 * Send dhcpack message, reason is a string stating why the dhcpack is being sent,
 * for example is a response to dhcpinform or whether its acknowledging new 
 * address or renewing lease on already leased address. This string is used 
 * in a log to differentiate scenarios on which the message was sent
 */
int message_dhcpack_send(dhcp_server_t *server, dhcp_message_t *message, const char *reason);

int message_dhcpack_build(dhcp_server_t *server, dhcp_message_t *dhcp_request, 
    uint32_t offered_lease_duration, uint32_t leased_address);

int message_dhcpack_build_lease_renew(dhcp_server_t *server, dhcp_message_t *request);
#endif // !__DHCPACK_H__

