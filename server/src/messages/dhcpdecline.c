#include "dhcpdecline.h"
#include "../logging.h"
#include "cclog_macros.h"
#include "../utils/xtoy.h"

int message_dhcpdecline_handle(dhcp_server_t *server, dhcp_message_t *message)
{
        if (!server || !message)
                return -1;

        int rv = -1;

        dhcp_message_t *ack = trans_cache_retrieve_message_last(server->trans_cache, 
                        message->xid, DHCP_ACK);
        if_null_log(ack, exit, LOG_WARN, NULL,
                        "Handling dhcpdecline failed because no dhcpack found in transaction");

        if_true((ack->yiaddr == 0), exit);
        cclog(LOG_WARN, NULL, "Possible misconfiguration: Received DHCPDECLINE on address %s", 
                        uint32_to_ipv4_address(ack->yiaddr));

        // TODO: set misconfigured flag on lease (when lease flags are implemented)

        rv = 0;
exit:
        return rv;
}

