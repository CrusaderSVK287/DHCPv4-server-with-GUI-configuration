#include "dhcp_snoop.h"
#include "../../dhcp_packet.h"
#include "../../RFC/RFC-2131.h"
#include "../../RFC/RFC-1700.h"
#include "../../logging.h"
#include "../../utils/xtoy.h"
#include "../../dhcp_options.h"
#include "../../timer.h"
#include "../../utils/llist.h"
#include <cclog.h>
#include <cclog_macros.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>

#define STATUS_MSG_LEN 4096

static int keep_waiting_for_offer = 0;

static int discover_send(dhcp_server_t *server, dhcp_message_t *discover)
{
        if (!server || !discover)
                return -1;

        int rv = -1;

        struct sockaddr_in addr = {0};
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        /* We are sending as a CLIENT, which sends on port 68 instead of 67 */
        addr.sin_port = htons(68);
        addr.sin_addr.s_addr = 0xffffffff;

        if_failed_log_n(sendto(server->sock_fd, &discover->packet, sizeof(dhcp_packet_t), 0,
                        (struct sockaddr*)&addr, sizeof(addr)), 
                        exit, LOG_ERROR, NULL, "DHCP Snoop: Failed to send DISCOVER message: %s", 
                                                strerror(errno));

        rv = 0;              
exit:
        return rv;
}

static int get_dhcp_options(dhcp_message_t *discover)
{
        if (!discover)
                return -1;

        int rv = -1;

        uint8_t val = DHCP_DISCOVER;
        dhcp_option_t *o = dhcp_option_new_values(DHCP_OPTION_DHCP_MESSAGE_TYPE,
                                                  1,
                                                  &val);
        char *hostname = "xyasvewvews";
        dhcp_option_t *o_hostname = dhcp_option_new_values(DHCP_OPTION_HOST_NAME,
                                                  strlen(hostname),
                                                  hostname);
        uint8_t client_id[7] = {1, discover->chaddr[0],discover->chaddr[1],discover->chaddr[2],
                discover->chaddr[3],discover->chaddr[4],discover->chaddr[5]};
        dhcp_option_t *o_client_id = dhcp_option_new_values(DHCP_OPTION_CLIENT_IDENTIFIER,
                                                  7,
                                                  client_id);
        uint8_t parameter_request[] = {1, 3, 6, 12, 15, 51, 54};
        dhcp_option_t *o_paameters = dhcp_option_new_values(DHCP_OPTION_PARAMETER_REQUEST_LIST,
                                                            7,
                                                            parameter_request);

        if_failed_log(dhcp_option_add(discover->dhcp_options, o), exit, LOG_ERROR, NULL, 
                      "Cannot perform DHCP scan, failed to set up dhcp discover options");
        if_failed_log(dhcp_option_add(discover->dhcp_options, o_hostname), exit, LOG_ERROR, NULL, 
                      "Cannot perform DHCP scan, failed to set up dhcp discover options");
        if_failed_log(dhcp_option_add(discover->dhcp_options, o_client_id), exit, LOG_ERROR, NULL, 
                      "Cannot perform DHCP scan, failed to set up dhcp discover options");
        if_failed_log(dhcp_option_add(discover->dhcp_options, o_paameters), exit, LOG_ERROR, NULL, 
                      "Cannot perform DHCP scan, failed to set up dhcp discover options");

        rv = 0;
exit:
        return rv;
}

static int snoop_send_dhcp_discover(dhcp_server_t *server, const char *spoofed_mac, uint32_t *xid_buf)
{
        if(!server || !spoofed_mac)
                return -1;

        int rv = -1;

        dhcp_message_t *discover = dhcp_message_new();
        if_null(discover, exit);

        discover->opcode = BOOTREQUEST;
        discover->htype  = ETHERNET;
        discover->hlen   = 6;
        discover->hops   = 0;
        // TODO: check whether our server catches this message after the scan. If yes, 
        // create a rule to ignore this message based on xid
        srand(time(NULL));
        discover->xid    = ((uint32_t)rand() << 16) | rand();
        discover->secs   = 0;
        discover->ciaddr = 0;
        discover->yiaddr = 0;
        discover->siaddr = 0;
        discover->flags  = 0; // We set broadcast flag 
        discover->giaddr = 0;
        discover->cookie = MAGIC_COOKIE;
        discover->type   = DHCP_DISCOVER;

        uint8_t mac_buffer[6];
        mac_to_uint8_array(spoofed_mac, mac_buffer);
        memcpy(discover->chaddr, mac_buffer, 6);
        if_failed(get_dhcp_options(discover), exit);

        if_failed(dhcp_packet_build(discover), exit);
        dhcp_packet_dump(&discover->packet);
        if_failed(discover_send(server, discover), exit);

        rv = 0;
        *xid_buf = discover->xid;
exit:
        return rv;
}

static int cb_turn_off_scanning(uint32_t time, void *priv)
{
        keep_waiting_for_offer = 0;
        return 0;
}

static bool is_server_whitelisted(llist_t *list, uint32_t siaddr)
{
        if (!list)
                return false;

        llist_foreach(list, {
                if (*(uint32_t*)node->data == siaddr)
                        return true;
        })

        return false;
}

static enum dhcp_snooper_status listen_to_offers(dhcp_server_t *server, 
                                                llist_t *server_whitelist,
                                                char *status_msg,
                                                uint32_t xid)
{
        if (!server || !status_msg)
            return DHCP_SNOOP_ERROR;

        int rv = DHCP_SNOOP_ERROR;

        keep_waiting_for_offer = 1;
        struct timer *timer = timer_new(TIMER_ONCE, 10, true, cb_turn_off_scanning);
        if_null(timer, exit);
        dhcp_message_t *message = dhcp_message_new();
        if_null(message, exit);

        int bytes = 0;
        char buff[256];
        while (keep_waiting_for_offer) {
                timer_update(timer, NULL);
        
                bytes = recv(server->sock_fd, &message->packet, sizeof(dhcp_packet_t), 0);
		if (bytes < 0 && errno == EAGAIN) {
			continue;
		} else if (bytes < 0) {
			cclog(LOG_WARN, NULL, "Failed to receive dhcp packet with return code %d", bytes);
			continue;
		}

                if (dhcp_packet_parse(message) < 0)
                        continue;

                if (message->type != DHCP_OFFER || 
                    message->xid  != xid)
                        continue;

                if (is_server_whitelisted(server_whitelist, message->siaddr))
                        continue;

                sprintf(buff, "THREAT:%s,", uint32_to_ipv4_address(message->siaddr));
                strcat(status_msg, buff);
                cclog(LOG_ERROR, NULL, "Potential rogue DHCP server on address %s", uint32_to_ipv4_address(message->siaddr));
                rv = DHCP_SNOOP_POTENTIAL_ROGUE;
        }
        
        if (rv != DHCP_SNOOP_POTENTIAL_ROGUE) {
                strcat(status_msg, "NO_THREAT");
                rv = DHCP_SNOOP_NO_THREAT;
        }
exit:
        return rv;
}

enum dhcp_snooper_status dhcp_snooper_perform_scan(dhcp_server_t *server, 
                                                   const char *spoofed_mac,
                                                   llist_t *server_whitelist,
                                                   char *status_msg)
{
#ifndef CONFIG_SECURITY_ENABLE_DHCP_SNOOPING
        return DHCP_SNOOP_DISABLED;
#endif /* ifndef CONFIG_SECURITY_ENABLE_DHCP_SNOOPING */

        /*
         * Send a dhcpdiscover, wait x seconds for response.
         * If response is received, if there are whitelisted 
         * dhcp server, check whether they answered, if not, 
         * raise allert with information about the infringing server.
         */

        if (!server || !spoofed_mac)
                return DHCP_SNOOP_ERROR;

        int rv = DHCP_SNOOP_ERROR;
        uint32_t xid = 0;

        if (status_msg) {
                free(status_msg);
        }
        status_msg = malloc(STATUS_MSG_LEN);
        if_null_log(status_msg, exit, LOG_ERROR, NULL, "Failed to allocate space for status msg");
        memset(status_msg, 0, STATUS_MSG_LEN);

        if_failed_log(snoop_send_dhcp_discover(server, spoofed_mac, &xid), exit, LOG_ERROR, NULL,
                      "DHCP spoofing: Failed to send fake DHCPDISCOVER message");

        rv = listen_to_offers(server, server_whitelist, status_msg, xid);
        printf("%s\n", status_msg);
exit:
        return rv;
}

