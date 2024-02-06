
#include "dhcpinform.h"

int message_dhcpinform_handle(dhcp_server_t *server, dhcp_message_t *message)
{
        if (!server || !message)
                return -1;

        // TODO: Make dhcpack sending API more generic to enable it to differentiate 
        // between broadcast/unicast and to not send lease time duration if responding 
        // to dhcpinform

        return 0;
}

