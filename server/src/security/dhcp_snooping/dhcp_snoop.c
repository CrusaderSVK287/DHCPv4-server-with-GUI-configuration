#include "dhcp_snoop.h"
#include "../../dhcp_packet.h"
#include "../../RFC/RFC-2131.h"
#include "../../RFC/RFC-1700.h"
#include "../../logging.h"
#include "../../utils/xtoy.h"
#include "../../dhcp_options.h"
#include "../../timer.h"
#include "../../utils/llist.h"
#include <asm-generic/socket.h>
#include <cclog.h>
#include <cclog_macros.h>
#include <netinet/in.h>
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

static int discover_send(int socket, dhcp_message_t *discover)
{
        if (!discover)
                return -1;

        int rv = -1;

        struct sockaddr_in addr = {0};
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        /* We are sending as a CLIENT, which sends to port 67 instead of 68 */
        addr.sin_port = htons(67);
        addr.sin_addr.s_addr = 0xffffffff;

        if_failed_log_n(sendto(socket, &discover->packet, sizeof(dhcp_packet_t), 0,
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
        // char *hostname = "xyasvewvews";
        // dhcp_option_t *o_hostname = dhcp_option_new_values(DHCP_OPTION_HOST_NAME,
        //                                           strlen(hostname),
        //                                           hostname);
        // uint8_t client_id[7] = {1, discover->chaddr[0],discover->chaddr[1],discover->chaddr[2],
        //         discover->chaddr[3],discover->chaddr[4],discover->chaddr[5]};
        // dhcp_option_t *o_client_id = dhcp_option_new_values(DHCP_OPTION_CLIENT_IDENTIFIER,
        //                                           7,
        //                                           client_id);
        // uint8_t parameter_request[] = {1, 3, 6, 12, 15, 51, 54};
        // dhcp_option_t *o_paameters = dhcp_option_new_values(DHCP_OPTION_PARAMETER_REQUEST_LIST,
        //                                                     7,
                                                            // parameter_request);

        if_failed_log(dhcp_option_add(discover->dhcp_options, o), exit, LOG_ERROR, NULL, 
                      "Cannot perform DHCP scan, failed to set up dhcp discover options");
        // if_failed_log(dhcp_option_add(discover->dhcp_options, o_hostname), exit, LOG_ERROR, NULL, 
        //               "Cannot perform DHCP scan, failed to set up dhcp discover options");
        // if_failed_log(dhcp_option_add(discover->dhcp_options, o_client_id), exit, LOG_ERROR, NULL, 
        //               "Cannot perform DHCP scan, failed to set up dhcp discover options");
        // if_failed_log(dhcp_option_add(discover->dhcp_options, o_paameters), exit, LOG_ERROR, NULL, 
        //               "Cannot perform DHCP scan, failed to set up dhcp discover options");

        rv = 0;
exit:
        return rv;
}

static int snoop_send_dhcp_discover(int socket, const char *spoofed_mac, uint32_t *xid_buf)
{
        if(!spoofed_mac)
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
        discover->xid    = rand();
        discover->secs   = htons(65535);
        discover->ciaddr = 0;
        discover->yiaddr = 0;
        discover->siaddr = 0;
        discover->flags  = 32768; // We set broadcast flag 
        discover->giaddr = 0;
        discover->cookie = MAGIC_COOKIE;
        discover->type   = DHCP_DISCOVER;

        uint8_t mac_buffer[6];
        mac_to_uint8_array(spoofed_mac, mac_buffer);
        memcpy(discover->chaddr, mac_buffer, 6);
        if_failed(get_dhcp_options(discover), exit);

        if_failed(dhcp_packet_build(discover), exit);
        if_failed(discover_send(socket, discover), exit);

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

static bool is_server_whitelisted(llist_t *list, dhcp_message_t *m)
{
        if (!list || !m)
                return false;

        dhcp_option_t *o = dhcp_option_retrieve(m->dhcp_options, DHCP_OPTION_SERVER_IDENTIFIER);
        if (!o)
                return false;

        llist_foreach(list, {
                if (*(uint32_t*)node->data == o->value.ip)
                        return true;
        })

        return false;
}

static void log_threat(dhcp_message_t *msg, char *status)
{
        if (!msg || !status)
                return;
        
        char buff[512];
        
        dhcp_option_t *o_server_id = dhcp_option_retrieve(msg->dhcp_options, DHCP_OPTION_SERVER_IDENTIFIER);
        if (!o_server_id) {
                sprintf(buff, "T: No_ID");
        } else {
                sprintf(buff, "T: %s", uint32_to_ipv4_address(o_server_id->value.ip));
        }
        strcat(status, buff);
        dhcp_option_t *o_hostname = dhcp_option_retrieve(msg->dhcp_options, DHCP_OPTION_HOST_NAME);
        if (!o_server_id) {
                sprintf(buff, " No_host,");
        } else {
                sprintf(buff, " host %s,", o_hostname->value.string);
        }
        strcat(status, buff);

        cclog(LOG_WARN, NULL, "Potential rogue DHCP server. ID: %s, hostname: %s",
              o_server_id ? uint32_to_ipv4_address(o_server_id->value.ip) : "not provided",
              o_hostname  ? o_hostname->value.string: "not provided");
}

static enum dhcp_snooper_status listen_to_offers(int socket, 
                                                llist_t *server_whitelist,
                                                char *status_msg,
                                                uint32_t xid)
{
        if (!status_msg)
            return DHCP_SNOOP_ERROR;

        int rv = DHCP_SNOOP_ERROR;

        keep_waiting_for_offer = 1;
        struct timer *timer = timer_new(TIMER_ONCE, 10, true, cb_turn_off_scanning);
        if_null(timer, exit);
        dhcp_message_t *message = dhcp_message_new();
        if_null(message, exit);

        int bytes = 0;
        while (keep_waiting_for_offer) {
                timer_update(timer, NULL);
        
                bytes = recv(socket, &message->packet, sizeof(dhcp_packet_t), 0);
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

                if (is_server_whitelisted(server_whitelist, message))
                        continue;

                log_threat(message, status_msg);
                rv = DHCP_SNOOP_POTENTIAL_ROGUE;
        }
        
        if (rv != DHCP_SNOOP_POTENTIAL_ROGUE) {
                strcat(status_msg, "NO_THREAT");
                rv = DHCP_SNOOP_NO_THREAT;
        }
exit:
        return rv;
}

static int create_dhcp_client_socket(dhcp_server_t *server) 
{
        if (!server)
                return -1;

        int sock = -1;
        int flag = 0;
        struct sockaddr_in caddr = {0};
        memset(&caddr, 0, sizeof(caddr));
        if_failed_n((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)), exit);

        caddr.sin_family = AF_INET;
        caddr.sin_port = htons(68);
        caddr.sin_addr.s_addr = INADDR_ANY; /* listen on any address */
        memset(&caddr.sin_zero, 0, sizeof(caddr.sin_zero));

        flag = 1;
        if_failed(setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag)), exit);
        if_failed(setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &flag, sizeof(flag)), exit);
        if_failed(setsockopt(sock, SOL_SOCKET, SO_BINDTODEVICE, server->config.interface, strlen(server->config.interface)), exit);

	int flags = fcntl(sock, F_GETFL, 0);
	if_failed_log_n(flags, exit, LOG_CRITICAL, NULL, "Failed to obtain socket fd flags");

	if_failed_log(fcntl(sock, F_SETFL, flags | O_NONBLOCK), 
		exit, LOG_CRITICAL, NULL, "Failed to set socket to non-blocking state");

        if_failed(bind(sock, (struct sockaddr *)&caddr, sizeof(caddr)), exit);
exit:
        return sock;
}

enum dhcp_snooper_status dhcp_snooper_perform_scan(dhcp_server_t *server, 
                                                   const char *spoofed_mac,
                                                   llist_t *server_whitelist,
                                                   char **status_msg)
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

        if (!server || !spoofed_mac || !status_msg)
                return DHCP_SNOOP_ERROR;

        int rv = DHCP_SNOOP_ERROR;
        uint32_t xid = 0;

        if (*status_msg) {
                free(*status_msg);
        }
        *status_msg = malloc(STATUS_MSG_LEN);
        if_null_log(*status_msg, exit, LOG_ERROR, NULL, "Failed to allocate space for status msg");
        memset(*status_msg, 0, STATUS_MSG_LEN);

        int socket = create_dhcp_client_socket(server);
        if_failed_log_n(socket, exit, LOG_ERROR, NULL, "Failed to create dhcp client socket");

        if_failed_log(snoop_send_dhcp_discover(socket, spoofed_mac, &xid), exit, LOG_ERROR, NULL,
                      "DHCP spoofing: Failed to send fake DHCPDISCOVER message");

        rv = listen_to_offers(socket, server_whitelist, *status_msg, xid);
        close(socket);
        printf("%s\n", *status_msg);

exit:
        return rv;
}

