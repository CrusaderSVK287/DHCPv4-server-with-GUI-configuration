#include "database.h"
#include "dhcp_packet.h"
#include "logging.h"
#include "transaction.h"
#include "utils/xtoy.h"
#include <cclog.h>
#include <cclog_macros.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <dirent.h>
#include <stdlib.h>

#define DB_FILE_PATH_FORMAT "/var/dhcp/database/%08x_%02x%02x%02x%02x%02x%02x.dhcp"
#define METADATA_FORMAT "T:%08lxCA:%02x%02x%02x%02x%02x%02xP:"
#define METADATA_LEN 128

int database_store_message(dhcp_message_t *message)
{
        if (!message)
                return -1;

        int rv = -1;

        char path[PATH_MAX];
        char buff[METADATA_LEN + 1];
        snprintf(path, PATH_MAX, DB_FILE_PATH_FORMAT, message->xid,
                message->chaddr[0],
                message->chaddr[1],
                message->chaddr[2],
                message->chaddr[3],
                message->chaddr[4],
                message->chaddr[5]);

        int fd = open(path, O_RDWR | O_APPEND | O_CREAT, 0644);
        if_failed_log_n(fd, exit, LOG_WARN, NULL, "Failed to open database file %s: %s", path, strerror(errno));

        memset(buff, 0, METADATA_LEN);
        snprintf(buff, METADATA_LEN, METADATA_FORMAT, time(NULL),
                message->chaddr[0],
                message->chaddr[1],
                message->chaddr[2],
                message->chaddr[3],
                message->chaddr[4],
                message->chaddr[5]);
        
        if_failed_log_n((rv = write(fd, buff, METADATA_LEN)), exit, LOG_WARN, NULL, "Failed to write message "
                        "from %lx metadata to database. %s", time(NULL), strerror(errno));
        if_failed_log_n(write(fd, &message->packet, sizeof(dhcp_packet_t)), exit, LOG_WARN, NULL, 
                "Failed to write message from %lx packet data to database. %s", message->xid, strerror(errno));

        rv = 0;
exit:
        if (fd >= 0) {
                close(fd);
        }
        return rv;
}

/* main api function */
transaction_t *database_load_transaction(uint32_t xid, uint8_t mac[6])
{
        if (!xid || !mac)
                return NULL;

        transaction_t *trans = trans_new(0);
        if_null_log(trans, error, LOG_ERROR, NULL, "Failed to allocate space for transaction");

        char path[PATH_MAX];
        snprintf(path, PATH_MAX, DB_FILE_PATH_FORMAT, xid, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        int fd = open(path, O_RDONLY);
        if_failed_log_n(fd, error, LOG_ERROR, NULL, "Failed to open database file: %s", strerror(errno));

        dhcp_message_t *message = dhcp_message_new();
        if_null_log(message, error, LOG_ERROR, NULL, "Cannot allocate space for dhcp message");

        char buff[BUFSIZ];
        int rv = 0;
        do {
                memset(buff, 0, BUFSIZ);
                if_failed_log_n((rv = read(fd, buff, METADATA_LEN)), error, LOG_ERROR, NULL, 
                        "Failed to read from database file %s. %s", path, strerror(errno));
                if_failed_log_n((rv = read(fd, &message->packet, sizeof(dhcp_packet_t))), error, LOG_ERROR, NULL, 
                        "Failed to read from database file %s. %s", path, strerror(errno));
                if (rv == 0)
                        break;

                if (sscanf(buff, "T:%08lx", (unsigned long *)&message->time) != 1) {
                        cclog(LOG_ERROR, NULL, "Corrupted metadata in transaction %lx", xid);
                        continue;
                }
                if_failed_log(dhcp_packet_parse(message), error, LOG_ERROR, NULL, "Failed to parse dhcp message from database");
                if_failed_log(trans_add(trans, message), error, LOG_ERROR, NULL, "Failed to load dhcp message to transaction");
        } while (true);

        return trans;
error:
        if (trans)
                trans_destroy(&trans);
        return NULL;
}

transaction_t *database_load_transaction_str(uint32_t xid, const char *mac)
{
        uint8_t mac_arr[6];
        mac_to_uint8_array(mac, mac_arr);
        return database_load_transaction(xid, mac_arr);
}

static transaction_t *database_find_xid_or_mac(uint32_t xid, uint8_t mac[6], bool find_xid)
{
        DIR *dir;
        struct dirent *entry;

        transaction_t *rv = NULL;

        dir = opendir("/var/dhcp/database");
        if_null_log(dir, exit, LOG_ERROR, NULL, "Failed to open directory /var/dhcp/database: %s", strerror(errno));

        uint32_t dxid;
        char dmac[32];
        char fmac[32];
        bool found = false;
        snprintf(fmac, 32, "%02x%02x%02x%02x%02x%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        
        while ((entry = readdir(dir)) != NULL) {
                memset(dmac, 0, 32);
                if (sscanf(entry->d_name, "%08x_%[a-f0-9].dhcp", &dxid, dmac) != 2)
                        continue;

                /* if we are finding xid, we check mac */
                if (find_xid && !strncmp(fmac, dmac, 12)) {
                        xid = dxid;
                        found = true;
                        break;
                } else if (dxid == xid) {
                        sscanf(dmac, "%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx",
                                &mac[0], &mac[1], &mac[2],
                                &mac[3], &mac[4], &mac[5]);
                        found = true;
                        break;
                }
        }

        if_false_log(found, exit, LOG_ERROR, NULL, "Cannot find complementary file to eighter xid %lx or mac %s", 
                     xid, uint8_array_to_mac(mac));

        rv = database_load_transaction(xid, mac);
exit:
        if(dir) {
                closedir(dir);
        }

        return rv;
}

/* need to find mac */
transaction_t *database_load_transaction_xid(uint32_t xid)
{
        uint8_t mac[6] = {0,0,0,0,0,0};
        return database_find_xid_or_mac(xid, mac, false);
}

/* need to find xid need to find xid  */
transaction_t *database_load_transaction_mac(uint8_t mac[6])
{
        return database_find_xid_or_mac(0, mac, true);
}

transaction_t *database_load_transaction_mac_str(const char *mac)
{
        uint8_t mac_arr[6];
        mac_to_uint8_array(mac, mac_arr);
        return database_load_transaction_mac(mac_arr);
}

