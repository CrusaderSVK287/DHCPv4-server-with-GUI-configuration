#include "dhcprequest.h"
#include "dhcpack.h"
#include "dhcpnak.h"
#include "../utils/xtoy.h"
#include "../logging.h"
#include "../lease.h"
#include "cclog.h"
#include "cclog_macros.h"
#include "../allocator.h"
#include <alloca.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

static int dhcp_requst_commit_lease(dhcp_server_t *server, dhcp_message_t *request, 
                uint32_t lease_time)
{
        if (!server || !request)
                return DHCP_REQUEST_ERROR;

        int rv = DHCP_REQUEST_COMMIT_ERROR;

        address_pool_t *pool = allocator_get_pool_by_address(server->allocator, request->yiaddr);

        lease_t *lease = lease_new();
        if_null(lease, exit);

        lease->xid = request->xid;
        // TODO: Implement flags
        lease->flags = 0;
        lease->address = request->yiaddr;
        lease->subnet = pool->mask;
        lease->lease_start = time(NULL);
        lease->lease_expire = lease->lease_start + lease_time;
        lease->pool_name = pool->name;
        memcpy(lease->client_mac_address, request->chaddr, 6);

        if_failed_log(lease_add(lease), error, LOG_ERROR, NULL, "Failed to commit lease of address"
                        " %s from pool %s", uint32_to_ipv4_address(lease->address), pool->name);
        
        rv = DHCP_REQUEST_OK;
error:
        lease_destroy(&lease);
exit:
        return rv;
}

static int dhcp_request_response_to_offer(dhcp_server_t *server, dhcp_message_t *request, 
                dhcp_option_t *o54)
{
        if (!server || !request || !o54)
                return -1;

        int rv = DHCP_REQUEST_ERROR;
        
        /* Check server identifier option */
        if (o54->value.ip != ipv4_address_to_uint32("192.168.0.250")) {
                cclog(LOG_INFO, NULL, "Received request, but with different server identifier");
                rv = DHCP_REQUEST_DIFFERENT_SERVER_IDENTIFICATOR;
                goto exit;
        }

        /* TODO: make more robust validation (will be implemented with the cache) */
        /* dhcp message validation */
        rv = DHCP_REQUEST_INVALID;
        if_false((request->ciaddr == 0), exit);
        if_false((request->yiaddr != 0), exit);
        if (allocator_is_address_available(server->allocator, request->yiaddr)) { 
                cclog(LOG_WARN, NULL, "Received request for address that is not offered");
                goto exit;
        }
        dhcp_option_t *o51 = dhcp_option_retrieve(server->allocator->default_options, 
                                                DHCP_OPTION_IP_ADDRESS_LEASE_TIME);
        if_null(o51, exit);

        /* All good, we can proceed and commit configuration */ 
        if_failed_log_n(dhcp_requst_commit_lease(server, request, o51->value.number), 
                exit, LOG_ERROR, NULL, 
                "Failed to commit lease of address %s", uint32_to_ipv4_address(request->yiaddr));

        rv = DHCP_REQUEST_OK;
exit:
        return rv;
}

static int dhcp_request_renew_lease(dhcp_server_t *server, dhcp_message_t *request)
{
        return 0;
}

int message_dhcprequest_handle(dhcp_server_t *server, dhcp_message_t *request)
{
        if (!server || !request)
                return -1;

        int rv = -1;

        dhcp_option_t *o54 = dhcp_option_retrieve(request->dhcp_options, DHCP_OPTION_SERVER_IDENTIFIER);
        
        /* 
         * According to RFC-2131, if client sends option 54 in dhcprequest, it is requesting new 
         * address. If not, its eighter rebinding or renewing
         */
        if (o54) {
                if (dhcp_request_response_to_offer(server, request, o54) < 0) {
                        // ERROR, send NAK
                } else {
                        if_failed_n(message_dhcpack_build(server, request, o54->value.number), exit);
                }
        } else {
                if_failed_n(dhcp_request_renew_lease(server, request), exit);           
        }


        rv = 0;
exit:
        return rv;
}

