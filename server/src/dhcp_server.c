#include "dhcp_server.h"
#include "RFC/RFC-2131.h"
#include "RFC/RFC-2132.h"
#include "address_pool.h"
#include "allocator.h"
#include "dhcp_options.h"
#include "dhcp_packet.h"
#include "lease.h"
#include "logging.h"
#include "messages/dhcp_messages.h"
#include "timer.h"
#include "transaction.h"
#include "utils/llist.h"
#include "utils/xtoy.h"
#include "transaction_cache.h"

#include <arpa/inet.h>
#include <asm-generic/socket.h>
#include <stdint.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>

#define BACKLOG_SIZE 50

static volatile int server_keep_running = 1;
static void stop_running(int signo)
{
	server_keep_running = 0;
}

int init_dhcp_server(dhcp_server_t *server)
{
	int rv = -1;

	if_null_log(server, exit, LOG_INFO, NULL, "server parameter is null");

	server->sock_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if_failed_log_n(server->sock_fd, exit, LOG_CRITICAL, NULL, 
		"Failed to create socket with return code %d", server->sock_fd);

	/* Setting socket option to reuse address */
	int reuse_addr = 1;
	if_failed_log(setsockopt(server->sock_fd, SOL_SOCKET, SO_REUSEADDR, &reuse_addr, sizeof(int)), 
		exit, LOG_CRITICAL, NULL, "Failed to set reuse address socket option");

	/* Setting socket option to reuse address */
	int broadcast = 1;
	if_failed_log(setsockopt(server->sock_fd, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(int)), 
		exit, LOG_CRITICAL, NULL, "Failed to set broadcast socket option");
	
        /* Setting up non-blocking socket */
	int flags = fcntl(server->sock_fd, F_GETFL, 0);
	if_failed_log_n(flags, exit, LOG_CRITICAL, NULL, "Failed to obtain socket fd flags");

	if_failed_log(fcntl(server->sock_fd, F_SETFL, flags | O_NONBLOCK), 
		exit, LOG_CRITICAL, NULL, "Failed to set socket to non-blocking state");

	struct sockaddr_in addr = {0};
	addr.sin_family = AF_INET;
	/* TODO: Make it possible to specify by user eighter address or interface which to use */
	//addr.sin_addr.s_addr = inet_addr("192.168.1.100");
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(67);

	if_failed_log(bind(server->sock_fd, (struct sockaddr*)&addr, sizeof(struct sockaddr_in)),
		exit, LOG_CRITICAL, NULL, "Failed to bind socket to address");

	cclog(LOG_MSG, NULL, "Server socket successfully initialised");

	/* Setting up sigint signal for proper shutdown */
	if (signal(SIGINT, stop_running) == SIG_ERR) { 
		cclog(LOG_CRITICAL, NULL, "Failed to set signal handler");
		goto exit;
	}
	cclog(LOG_MSG, NULL, "Signal handler set successfully");

	rv = 0;
exit:
	return rv;
}

/* Check leases in particular pool, returns number of successfully removed leases */
static int check_lease(uint32_t current_time, address_pool_t *pool, dhcp_server_t *server)
{
        lease_t lease;
        int released_leases = 0;

        /*
         * We will not skip to the end of function in case of error, because we want to loop 
         * through every address 
         */
        for (uint32_t a = pool->start_address; a < pool->end_address; a++) {
                if (allocator_is_address_available(server->allocator, a))
                        continue;
                
                if (lease_retrieve(&lease, a, pool->name) < 0) {
                        cclog(LOG_WARN, NULL, 
                                "Failed to retrieve lease of address %s, possible missconfiguration",
                                uint32_to_ipv4_address(a));
                        continue;
                }

                if (lease.lease_expire > current_time)
                        continue;

                /*
                 * These releases are not vital to working of server, so a warning on failure 
                 * is sufficient. Furthermore, we should NOT release address to pool in case 
                 * the lease fails to be removed 
                 */
                if (lease_remove(&lease) < 0) {
                        cclog(LOG_WARN, NULL, 
                                "Failed to remove lease of address %s", uint32_to_ipv4_address(a));
                        continue;
                }

                if (allocator_release_address(server->allocator, a) < 0) {
                        cclog(LOG_WARN, NULL, "Failed to release address %s to pool", 
                                        uint32_to_ipv4_address(a));
                }

                cclog(LOG_INFO, NULL, "Released lease of address %s from pool %s", 
                                uint32_to_ipv4_address(a), pool->name);
                released_leases++;
        }

        return released_leases;
}

int check_lease_expirations(uint32_t check_time, void *priv)
{
        if (!priv)
                return -1;

        int rv = -1;
        int released_leases = 0;
        dhcp_server_t *server = (dhcp_server_t*)priv;
        if_null(server, exit);

        address_pool_t *pool = NULL;
        llist_foreach(server->allocator->address_pools, 
                pool = (address_pool_t*)node->data;
                if (!pool)
                        continue;

                released_leases += check_lease(check_time, pool, server);
        )

        rv = released_leases;
exit:
        return rv;
}

int init_dhcp_server_timers(dhcp_server_t *server)
{
        if (!server)
                return -1;

        int rv = -1;

        // TODO: config, lease expiration check 
        server->timers.lease_expiration_check = timer_new(TIMER_REPEAT, 60, 
                                                true, check_lease_expirations);

        if_null_log(server->timers.lease_expiration_check, exit, LOG_CRITICAL, NULL, 
                        "Failed to initialise lease expiration check timer");

        cclog(LOG_MSG, NULL, "Initialised dhcp server timers");
        rv = 0;
exit:
        return rv;
}

int uninit_dhcp_server(dhcp_server_t *server)
{
	int rv = -1;

	if_null_log(server, exit, LOG_INFO, NULL, "server parameter is null");

	if_failed_log(close(server->sock_fd), exit, LOG_ERROR, NULL, "Failed to close socket");
        allocator_destroy(&server->allocator);
        trans_cache_destroy(&server->trans_cache);

        timer_destroy(&server->timers.lease_expiration_check);

	cclog(LOG_MSG, NULL, "Server stoped successfully");
	rv = 0;
exit:
	return rv;
}

void update_timers(dhcp_server_t *server)
{
        /* Update timers on transaction cache entries. */
        for (int i = 0; i < server->trans_cache->size; i++) {
                if (server->trans_cache->transactions[i]->timer->is_running)
                        trans_update_timer(server->trans_cache->transactions[i]);
        }

        int released_leases = timer_update(server->timers.lease_expiration_check, server);
        if (released_leases == TIMER_ERROR)
                cclog(LOG_WARN, NULL, "Failed to update lease expiration check timer");
        else if (released_leases > 0)
                cclog(LOG_MSG, NULL, "Released %d addresses from lease database", released_leases);

        // TODO: Maybe do a configuration to control whether or not to lower cpu load / set the delay
        /* Introduce a slight delay between processes in order to lower cpu load */
        // usleep(50000);
}

int dhcp_server_serve(dhcp_server_t *server)
{
	int rv = -1;

	if_null_log(server, exit, LOG_CRITICAL, NULL, "server parameter is null");

        dhcp_message_t *dhcp_msg = dhcp_message_new();
        if_null_log(dhcp_msg, exit, LOG_ERROR, NULL, "Cannot allocate memory for dhcp_message_t");

	do
	{
                /* Update various timers used by server (e.g. transaction cache timers )*/
                update_timers(server);        

		rv = recv(server->sock_fd, &dhcp_msg->packet, sizeof(dhcp_packet_t), 0);
		if (rv < 0 && errno == EAGAIN) {
			continue;
		} else if (rv < 0) {
			cclog(LOG_WARN, NULL, "Failed to receive dhcp packet with return code %d", rv);
			continue;
		}

                /* Parse the packet, errors in packet parsing are handled in the parse function */
                if (dhcp_packet_parse(dhcp_msg) < 0)
                        continue;

                /* If we capture a message sent by a server, drop it */
                if (dhcp_msg->type == DHCP_OFFER || 
                        dhcp_msg->type == DHCP_ACK || 
                        dhcp_msg->type == DHCP_NAK)
                        continue;

                /* Store the received message in cache for future use */
                cclog(LOG_MSG, NULL, "Received message of type %s from %s", 
                                rfc2131_dhcp_message_type_to_str(dhcp_msg->type),
                                uint8_array_to_mac((uint8_t*)dhcp_msg->chaddr));

                /*
                 * Store message in cache for future reference, drop communication if the message 
                 * cannot be stored, for example due to full cache (log should be made)
                 */
                if (trans_cache_add_message(server->trans_cache, dhcp_msg) < 0)
                        continue;
                
                switch (dhcp_msg->type) {
                        case DHCP_DISCOVER: 
                                if_failed_log_n_ng((rv = message_dhcpdiscover_handle(server, dhcp_msg)), 
                                        LOG_ERROR, NULL, 
                                        "Failed to handle DHCPDISCOVER message from %s ret %d",
                                        uint8_array_to_mac((uint8_t*)dhcp_msg->chaddr), rv); 
                                break;
                        case DHCP_REQUEST:  message_dhcprequest_handle(server, dhcp_msg);  break;
                        case DHCP_DECLINE:  message_dhcpdecline_handle(server, dhcp_msg);  break;
                        case DHCP_INFORM:   message_dhcpinform_handle(server, dhcp_msg);   break;
                        case DHCP_RELEASE:  message_dhcprelease_handle(server, dhcp_msg);  break;

                        default:
                                cclog(LOG_WARN, NULL, "Invalid DHCP message type received (%d), "
                                                "dropping message", dhcp_msg->type);
                                break;
                }

	} while (server_keep_running);

	rv = 0;
exit:
	return rv;
}

