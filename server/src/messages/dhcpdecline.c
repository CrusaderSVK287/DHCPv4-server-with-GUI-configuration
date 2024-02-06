#include "dhcpdecline.h"

int message_dhcpdecline_handle(dhcp_server_t *server, dhcp_message_t *message)
{
        if (!server || !message)
                return -1;


        // Depends on chache implementation, since message->ciaddr will be zero and no options 
        // will be passed, we need to match the decline to previous transaction

        return 0;
}

