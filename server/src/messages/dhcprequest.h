
#ifndef __DHCPREQUEST_H__
#define __DHCPREQUEST_H__

#include "../dhcp_server.h"
#include "../dhcp_packet.h"

enum dhcp_request_status {
    DHCP_REQUEST_COMMIT_ERROR = -3,
    DHCP_REQUEST_INVALID = -2,
    DHCP_REQUEST_ERROR = -1,
    DHCP_REQUEST_OK = 0,
    DHCP_REQUEST_DIFFERENT_SERVER_IDENTIFICATOR = 1,
};

/* Handle DHCPREQUEST messages */
int message_dhcprequest_handle(dhcp_server_t *server, dhcp_message_t *message);

#endif // !__DHCPREQUEST_H__

