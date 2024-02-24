
#include "dhcpinform.h"
#include "dhcpack.h"

int message_dhcpinform_handle(dhcp_server_t *server, dhcp_message_t *message)
{
        if (!server || !message)
                return -1;

        if (message_dhcpack_build_inform_response(server, message) < 0)
                return -1;

        return 0;
}

