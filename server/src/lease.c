#include "lease.h"
#include "address_pool.h"
#include "cclog.h"
#include "logging.h"
#include "utils/llist.h"
#include "utils/xtoy.h"
#include <asm-generic/errno-base.h>
#include <cclog_macros.h>
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <cJSON.h>
#include <cJSON_Utils.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

/* Allocate space for new lease */
lease_t *lease_new()
{
        lease_t *l = calloc(1, sizeof(lease_t));

        if_null_log_ng(l, LOG_ERROR, NULL, "Cannot allocate lease");
        l->pool_name = NULL;

        return l;
}

/* destroy allocated lease */
void lease_destroy(lease_t **l)
{
        if (!l || !(*l))
                return;

        free(*l);
        *l = NULL;
}

int lease_retrieve_address(lease_t *result, uint32_t addr, llist_t *pools)
{
        if (!result)
                return LEASE_ERROR;

        address_pool_t *p = NULL;
        char *pool_name = NULL;
        llist_foreach(pools, {
                p = (address_pool_t*)node->data;
                if (address_belongs_to_pool(p, addr)) {
                        pool_name = p->name;
                        break;
                }
        })

        return lease_retrieve(result, addr, pool_name);
}

static int init_leases_file(const char *path) 
{
        if (!path)
                return -1;

        int fd = open(path, O_CREAT | O_RDWR, 0644);
        if_failed_n(fd, error);

        cJSON *root = cJSON_CreateObject();
        if_null(root, error);
        cJSON_AddArrayToObject(root, "leases");

        char *json = cJSON_Print(root);
        cJSON_Delete(root);
        if_null(json, error);

        if_failed_log_n(write(fd, json, strlen(json)), error, LOG_ERROR, NULL, 
                        "Failed to initialise %s file", path);
        free(json);

        if_failed_log_n(lseek(fd, 0, SEEK_SET), error, LOG_ERROR, NULL, 
                        "Failed to seek to beggining of %s file", path);

        return fd;
error:
        if (fd >= 0) {
                close(fd);
        }
        return -1;
}

static cJSON* lease_load_lease_fila(int fd, const char *path)
{
        struct stat st = {0};
        if_failed_log_n(stat(path, &st), error, LOG_ERROR, NULL,
                        "Cannot stat file %s", path);

        char *leases_data = calloc(1, st.st_size + 32);
        
        /* Get entire json from the file */
        if_failed_log_n(read(fd, leases_data, st.st_size + 32), error, LOG_ERROR, NULL,
                        "Failed reading from %s", path);

        cJSON *result = cJSON_Parse(leases_data);

        free(leases_data);
        return result;
error:
        return NULL;;
}

static int lease_open_lease_file(const char *pool)
{
        if (!pool)
                return -1;

        char path[FILENAME_MAX];
        snprintf(path, FILENAME_MAX, LEASE_PATH_PREFIX "%s.lease", pool);

        int fd = open(path, O_RDWR);
        /* Initialise the file if it doesnt exist */
        if (fd < 0 && errno == ENOENT) {
                fd = init_leases_file(path);
        }

        return fd;
}

static cJSON* lease_to_cjson(lease_t *lease)
{
        if (!lease)
                return NULL;

        cJSON *address     = cJSON_CreateString(uint32_to_ipv4_address(lease->address));
        cJSON *subnet      = cJSON_CreateString(uint32_to_ipv4_address(lease->subnet));
        cJSON *xid         = cJSON_CreateNumber(lease->xid);
        cJSON *lease_start = cJSON_CreateNumber(lease->lease_start);
        cJSON *lease_end   = cJSON_CreateNumber(lease->lease_expire);
        cJSON *flags       = cJSON_CreateNumber(lease->flags);
        char mac_str[30];
        snprintf(mac_str, 30, "%02x:%02x:%02x:%02x:%02x:%02x",  lease->client_mac_address[0],
                                                lease->client_mac_address[1],
                                                lease->client_mac_address[2],
                                                lease->client_mac_address[3],
                                                lease->client_mac_address[4],
                                                lease->client_mac_address[5]);
        cJSON *mac = cJSON_CreateString(mac_str);

        if_null(address, exit);
        if_null(subnet, exit);
        if_null(xid, exit);
        if_null(lease_start, exit);
        if_null(lease_end, exit);
        if_null(flags, exit);
        if_null(mac, exit);

        cJSON *o = cJSON_CreateObject();
        if_null(o, exit);
        cJSON_AddItemToObject(o, "address", address);
        cJSON_AddItemToObject(o, "subnet", subnet);
        cJSON_AddItemToObject(o, "xid", xid);
        cJSON_AddItemToObject(o, "lease_start", lease_start);
        cJSON_AddItemToObject(o, "lease_expire", lease_end);
        cJSON_AddItemToObject(o, "flags", flags);
        cJSON_AddItemToObject(o, "client_mac_address", mac);

        return o;
exit:
        return NULL;
}

static int json_to_lease(lease_t *result, cJSON *json, const char *pool_name)
{
        if (!result || !json || !pool_name)
                return LEASE_ERROR;

        result->address      = ipv4_address_to_uint32(cJSON_GetStringValue(
                                cJSON_GetObjectItem(json, "address")));
        result->subnet       = ipv4_address_to_uint32(cJSON_GetStringValue(
                                cJSON_GetObjectItem(json, "subnet")));
        result->xid          = cJSON_GetNumberValue(cJSON_GetObjectItem(json, "xid"));
        result->lease_start  = cJSON_GetNumberValue(cJSON_GetObjectItem(json, "lease_start"));
        result->lease_expire = cJSON_GetNumberValue(cJSON_GetObjectItem(json, "lease_expire"));
        result->flags        = cJSON_GetNumberValue(cJSON_GetObjectItem(json, "flags"));
        const char *mac      = cJSON_GetStringValue(cJSON_GetObjectItem(json, "client_mac_address"));
        
        if_null(mac, error);

        unsigned int mac_byte0, mac_byte1, mac_byte2, mac_byte3, mac_byte4, mac_byte5;

        if (sscanf(mac, "%02x:%02x:%02x:%02x:%02x:%02x",
                        &mac_byte0, &mac_byte1, &mac_byte2,
                        &mac_byte3, &mac_byte4, &mac_byte5) == 6) {
                result->client_mac_address[0] = (uint8_t)mac_byte0;
                result->client_mac_address[1] = (uint8_t)mac_byte1;
                result->client_mac_address[2] = (uint8_t)mac_byte2;
                result->client_mac_address[3] = (uint8_t)mac_byte3;
                result->client_mac_address[4] = (uint8_t)mac_byte4;
                result->client_mac_address[5] = (uint8_t)mac_byte5;
        } else {
                cclog(LOG_ERROR, NULL, "Failed to parse mac addres from json");
                return LEASE_INVALID_JSON;
        }

        return LEASE_OK;
error:
        return LEASE_ERROR;
}


int lease_retrieve(lease_t *result, uint32_t addr, char *pool_name)
{
        if (!result || !pool_name)
                return LEASE_ERROR;

        int rv = LEASE_ERROR;

        char path[FILENAME_MAX];
        snprintf(path, FILENAME_MAX, LEASE_PATH_PREFIX "%s.lease", pool_name);
        
        int fd = lease_open_lease_file(pool_name);
        if_failed_log_n(fd, exit, LOG_ERROR , NULL, 
                        "Cannot retrieve lease, failed to open %s file", path);
     
        cJSON *root = lease_load_lease_fila(fd, path);
        cJSON *leases_array = cJSON_GetObjectItem(root, "leases");
        if_null(leases_array, exit);

        /* If lease dosent exist */
        rv = LEASE_DOESNT_EXITS;

        uint32_t cur_addr = 0;
        cJSON *element;
        cJSON_ArrayForEach(element, leases_array) {
                cur_addr = ipv4_address_to_uint32(cJSON_GetStringValue(
                                        cJSON_GetObjectItem(element, "address")));
                if (cur_addr == addr) {
                        rv = json_to_lease(result, element, pool_name);
                        break;
                }
        }

        close(fd);
        cJSON_Delete(root);

exit:
        return rv;
}

int lease_add(lease_t *lease)
{
        if (!lease || !lease->pool_name)
                return LEASE_ERROR;

        int rv = LEASE_ERROR;

        /* convert lease_t to json */
        cJSON *new_lease_json = lease_to_cjson(lease);
        if_null(new_lease_json, exit);

        char path[FILENAME_MAX];
        snprintf(path, FILENAME_MAX, LEASE_PATH_PREFIX "%s.lease", lease->pool_name);

        int fd = lease_open_lease_file(lease->pool_name);
        if_failed_log_n(fd, exit, LOG_ERROR , NULL, 
                        "Cannot add lease, failed to open %s file", path);

        
        cJSON *leases_root  = lease_load_lease_fila(fd, path);
        cJSON *leases_array = cJSON_GetObjectItem(leases_root, "leases");
        if_null(leases_array, exit);
        cJSON_AddItemToArray(leases_array, new_lease_json);
        
        char *json = cJSON_Print(leases_root);
        /* Seek and save data in file */
        if_failed_log_n(lseek(fd, 0, SEEK_SET), exit, LOG_ERROR, NULL, 
                        "Failed to seek to beggining of %s file", path);
        if_failed_log_n(write(fd, json, strlen(json)), exit, LOG_ERROR, NULL, 
                        "Failed to store lease data in %s file", path);
        
        free(json);
        close(fd);
        cJSON_Delete(leases_root);
        rv = LEASE_OK;
exit:
        return rv;
}

int lease_remove(lease_t *lease)
{
        if (!lease)
                return LEASE_ERROR;

        int rv = LEASE_ERROR;

        char path[FILENAME_MAX];
        snprintf(path, FILENAME_MAX, LEASE_PATH_PREFIX "%s.lease", lease->pool_name);
        
        int fd = lease_open_lease_file(lease->pool_name);
        if_failed_log_n(fd, exit, LOG_ERROR , NULL, 
                        "Cannot retrieve lease, failed to open %s file", path);
     
        cJSON *root = lease_load_lease_fila(fd, path);
        cJSON *leases_array = cJSON_GetObjectItem(root, "leases");
        if_null(leases_array, exit);

        /* If lease dosent exist */
        rv = LEASE_DOESNT_EXITS;

        uint32_t cur_addr = 0;
        cJSON *element;
        int index = 0;
        cJSON_ArrayForEach(element, leases_array) {
                cur_addr = ipv4_address_to_uint32(cJSON_GetStringValue(
                                        cJSON_GetObjectItem(element, "address")));
                if (cur_addr == lease->address) {
                        cJSON_DetachItemFromArray(leases_array, index);
                        cJSON_Delete(element);
                        rv = LEASE_OK;
                        break;
                }
                index++;
        }
        
        char *json = cJSON_Print(root);
        if_null(json, exit);
        /* Truncate the file to clear previous data and write new data without the lease */
        if_failed_log_n(ftruncate(fd, 0), exit, LOG_ERROR, NULL, 
                        "Failed to truncate %s file", path);
        if_failed_log_n(lseek(fd, 0, SEEK_SET), exit, LOG_ERROR, NULL, 
                        "Failed to seek to beggining of %s file", path);
        if_failed_log_n(write(fd, json, strlen(json)), exit, LOG_ERROR, NULL, 
                        "Failed to store lease data in %s file", path);

        free(json);
        close(fd);
        cJSON_Delete(root);
exit:
        return rv;
}

int lease_remove_address_pool(uint32_t address, char *pool_name)
{
        if (!pool_name)
                return LEASE_ERROR;

        lease_t l = {
                .address = address,
                .pool_name = pool_name,
        };

        return lease_remove(&l);
}

