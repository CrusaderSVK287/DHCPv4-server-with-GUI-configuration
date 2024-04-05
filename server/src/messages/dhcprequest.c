#include "dhcprequest.h"
#include "dhcpack.h"
#include "dhcpnak.h"
#include "../utils/xtoy.h"
#include "../logging.h"
#include "../lease.h"
#include "../allocator.h"
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <arpa/inet.h>

static int dhcp_requst_commit_lease(dhcp_server_t *server, dhcp_message_t *request, 
                uint32_t lease_time, uint32_t leased_address)
{
        if (!server || !request)
                return DHCP_REQUEST_ERROR;

        int rv = DHCP_REQUEST_COMMIT_ERROR;

        address_pool_t *pool = allocator_get_pool_by_address(server->allocator, leased_address);
        if_null(pool, exit);
        
        lease_t *lease = lease_new();
        if_null(lease, exit);

        lease->xid = request->xid;
        // TODO: Implement flags like static lease - reserved lease(permanent lease etc)
        lease->flags = 0;
        lease->address = leased_address;
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

static bool validate_client_id_and_requested_params(transaction_cache_t *cache, 
                dhcp_message_t *request)
{
        if (!cache || !request)
                goto error;

        dhcp_message_t *discover = trans_cache_retrieve_message(cache, request->xid, DHCP_DISCOVER);
        if_null(discover, error);

        /* Validate client identifier, it MUST be same in all messages */
        dhcp_option_t *request_o61 = dhcp_option_retrieve(request->dhcp_options, 
                        DHCP_OPTION_CLIENT_IDENTIFIER);
        dhcp_option_t *discover_o61 = dhcp_option_retrieve(discover->dhcp_options, 
                        DHCP_OPTION_CLIENT_IDENTIFIER);

        /* We only need to verify if the client sent option in both messages */
        if (request_o61 && discover_o61) {
                if_failed_log(memcmp(request_o61->value.binary_data, discover_o61->value.binary_data, 
                        request_o61->lenght), error, LOG_WARN, NULL, 
                        "Inconsistent client identifier, transaction will not proceed");
        }

        /* Validate parameter request list, it MUST be same in all messages */
        dhcp_option_t *request_o55 = dhcp_option_retrieve(request->dhcp_options, 
                        DHCP_OPTION_PARAMETER_REQUEST_LIST);
        dhcp_option_t *discover_o55 = dhcp_option_retrieve(discover->dhcp_options, 
                        DHCP_OPTION_PARAMETER_REQUEST_LIST);

        if (!request_o55 || !discover_o55) {
                cclog(LOG_WARN, NULL, "Parameter list was not found in a message, treating as invalid");
                goto error;
        }

        if_failed_log(memcmp(request_o55->value.binary_data, discover_o55->value.binary_data, 
                request_o55->lenght), error, LOG_WARN, NULL, 
                "Inconsistent parameter request list , transaction will not proceed");

        return true;
error:
        return false;
}

/*
 * Function tries to retrieve ip address from dhcp options of dhcprequest, if it 
 * fails, it tries to look for previously offered address instead.
 * Function returns 0 on error ! On success returns clients IP address
 */
static uint32_t retrieve_client_address(dhcp_message_t *request, transaction_cache_t *cache)
{
        if (!request || !cache)
                goto error;

        dhcp_option_t *o50 = dhcp_option_retrieve(request->dhcp_options, 
                        DHCP_OPTION_REQUESTED_IP_ADDRESS);

        /* If client didnt request address, look for previously offered address */
        if (!o50) {
                dhcp_message_t *offer = trans_cache_retrieve_message(cache, request->xid, DHCP_OFFER);
                if_null(offer, error);
                o50 = dhcp_option_retrieve(offer->dhcp_options, DHCP_OPTION_REQUESTED_IP_ADDRESS);
                if_null(o50, error);
        }

        return o50->value.ip;
error:
        return 0;
}

/*
 * Function retrieves lease time configured for the pool to which the address 
 * belongs. If the lease time is not configured there, it */
static uint32_t retrieve_lease_time(uint32_t address, address_allocator_t *allocator)
{
        if (!allocator || address == 0)
                goto error;

        address_pool_t *pool = allocator_get_pool_by_address(allocator, address);

        dhcp_option_t *o51 = dhcp_option_retrieve(pool->dhcp_option_override, 
                        DHCP_OPTION_IP_ADDRESS_LEASE_TIME);

        /* If the pool doesnt have lease time configured, fetch the global one */
        if (!o51) {
                o51 = dhcp_option_retrieve(allocator->default_options,
                                DHCP_OPTION_IP_ADDRESS_LEASE_TIME);
        }
        
        if_null_log(o51, error, LOG_ERROR, NULL, "Option 51 (lease time) is not configured");

        return o51->value.number;
error:
        return 0;
}

static int dhcp_request_response_to_offer(dhcp_server_t *server, dhcp_message_t *request, 
                dhcp_option_t *o54)
{
        if (!server || !request || !o54)
                return -1;

        int rv = DHCP_REQUEST_ERROR;
        
        /* Check server identifier option */
        if (o54->value.ip != server->config.bound_ip) {
                cclog(LOG_INFO, NULL, "Received request, but with different server identifier");
                rv = DHCP_REQUEST_DIFFERENT_SERVER_IDENTIFICATOR;
                goto exit;
        }

        /* dhcp message validation, ciaddr and yiaddr must be 0 if the request is response to offer */
        rv = DHCP_REQUEST_INVALID;
        if_false((request->ciaddr == 0), exit);
        if_false((request->yiaddr == 0), exit);
        if_false(validate_client_id_and_requested_params(server->trans_cache, request), exit);

        uint32_t requested_ip = retrieve_client_address(request, server->trans_cache);
        if_failed_log_ne(requested_ip, exit, LOG_WARN, NULL, "Cannot obtain requested IP address");

        if (allocator_is_address_available(server->allocator, requested_ip)) { 
                cclog(LOG_WARN, NULL, "Address %s is not currently being offered", 
                                uint32_to_ipv4_address(requested_ip));
                goto exit;
        }

        /* Retrieve lease time  */
        uint32_t lease_time = retrieve_lease_time(requested_ip, server->allocator);
        if_failed_ne(lease_time, exit);

        /* All good, we can proceed and commit lease */ 
        if_failed_log_n(dhcp_requst_commit_lease(server, request, lease_time, requested_ip), 
                exit, LOG_ERROR, NULL, 
                "Failed to commit lease of address %s", uint32_to_ipv4_address(requested_ip));

        rv = DHCP_REQUEST_OK;
exit:
        return rv;
}

static int dhcp_request_renew_lease(dhcp_server_t *server, dhcp_message_t *request)
{
        if (!server || !request)
                return -1;

        int rv = -1;

        /* If client doesnt specify its address, exit */
        if_false(request->ciaddr, exit);

        /* Retrieve lease from persistent database */
        lease_t lease = {0};
        if_failed(lease_retrieve_address(&lease, request->ciaddr, server->allocator->address_pools),
                exit);

        /* Verify client hardware address */
        if_failed(memcmp(lease.client_mac_address, request->chaddr, 6), exit);


        /* Delete existing lease */
        if_failed_log(lease_remove(&lease), exit, LOG_WARN, NULL, 
                "Failed step 1 of renewing lease of address %s", 
                uint32_to_ipv4_address(request->ciaddr));
        
        /* Set new expiration time for lease */
        lease.lease_start = time(NULL);
        lease.lease_expire = lease.lease_start + retrieve_lease_time(request->ciaddr, 
                                                        server->allocator);

        /* Renew lease */
        if_failed_log(lease_add(&lease), exit, LOG_WARN, NULL, 
                "Failed step 2 of renewing lease of address %s", 
                uint32_to_ipv4_address(request->ciaddr));

        rv = 0;
exit:
        return rv;
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
                // TODO: OPRAVIT - AK BUDE INY SERVER IDENTIFIER NEPOSIELAT ANI NAK ANI ACK
                if (dhcp_request_response_to_offer(server, request, o54) < 0) {
                        /* Error, send DHCPNAK to client, lease will be freed when transaction expires */
                        if_failed_n(message_dhcpnak_build(server, request), exit);
                } else {
                        /* Success, send DHCPACK to client */
                        dhcp_option_t *o50 = dhcp_option_retrieve(request->dhcp_options, 
                                        DHCP_OPTION_REQUESTED_IP_ADDRESS);
                        if_failed_n(message_dhcpack_build(server, request, o54->value.number, 
                                                o50->value.ip), exit);
                }
        } else {
                /*
                 * RFC-2131 states that the server SHOULD send dhcpack regardless of 
                 * whether it extended the lease or not
                 */
                if (request->ciaddr) {
                        dhcp_request_renew_lease(server, request);
                        if_failed(message_dhcpack_build_lease_renew(server, request), exit);
                }
        }

        rv = 0;
exit:
        return rv;
}

