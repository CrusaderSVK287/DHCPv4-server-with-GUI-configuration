#include "acl.h"
#include "../logging.h"
#include "cclog.h"
#include "cclog_macros.h"
#include <ctype.h>
#include <stdint.h>
#include <stdlib.h>
#include <cJSON.h>
#include <cJSON_Utils.h>
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include "../utils/xtoy.h"

ACL_t* ACL_new()
{
        ACL_t *acl = calloc(1, sizeof(ACL_t));
        if_null(acl, error);
        acl->entries = llist_new();
        if_null(acl->entries, error);

        return acl;
error:
        cclog(LOG_ERROR, NULL, "Failed to allocate memory for ACL");
        return NULL;
}

static cJSON *load_config_file(const char *path)
{
        struct stat st = {0};
        if (stat(path, &st) < 0) {
                cclog(LOG_ERROR, NULL, "Cannot stat file %s", path);
                goto error;
        }

        int fd = open(path, O_RDONLY);
        if (fd < 0) {
                cclog(LOG_ERROR, NULL, "Failed to open configuration file %s: %s\n", path, strerror(errno));
                goto error;
        }

        char *config = calloc(1, st.st_size + 32);
        
        /* Get entire json from the file */
        if (read(fd, config, st.st_size + 32) < 0) {
                cclog(LOG_ERROR, NULL, "Failed reading from %s: %s\n", path, strerror(errno));
                goto error;
        }

        cJSON *result = cJSON_Parse(config);

        free(config);
        return result;
error:
        return NULL;

}

static bool is_string_valid_mac(const char *s)
{
    size_t len = strlen(s);
    for (size_t i = 0; i < len; ++i) {
        if (!(isdigit(s[i]) || (toupper(s[i]) >= 'A' && toupper(s[i]) <= 'F') || s[i] == ':')) {
            return false;
        }
    }
    return (len == 17);
}

/* Function loads acl entries from file provided by path. */
int ACL_load_acl_entries(ACL_t *acl, const char *path)
{
        if (!acl || !path)
                return -1;

        int rv = ACL_OK;

        cJSON *config = load_config_file(path);
        if_null(config, exit);
        cJSON *config_security = cJSON_GetObjectItem(config, "security");
        if_null(config_security, exit_config);
        cJSON *config_acl_entries = cJSON_GetObjectItem(config_security, "acl_entries");
        if_null(config_acl_entries, exit_config);

        llist_clear(acl->entries);
        cJSON *entry = NULL;
        char *value;
        int i = 0;
        cJSON_ArrayForEach(entry, config_acl_entries) {
                if_false_log(cJSON_IsString(entry), error, LOG_WARN, NULL, "Bad config, ACL entries "
                                "must be strings");
                if_false_log(is_string_valid_mac(cJSON_GetStringValue(entry)), error, LOG_WARN, NULL, 
                                "Bad configuration format. ACL entries must be valid mac addresses");

                value = strdup(cJSON_GetStringValue(entry));
                for (i = 0; i < 17; i++) {
                        value[i] = tolower(value[i]);
                } 

                if_failed_log_ng(llist_append(acl->entries, value, true),
                                LOG_WARN, NULL, "Failed to add entry %s to ACL. Server will keep " 
                                "running but keep in mind this entry is not used", cJSON_GetStringValue(entry));
        }

exit_config:
        cJSON_Delete(config);
exit:
        return rv;
error:
        cclog(LOG_ERROR, NULL, "Error while loading acl entries");
        rv = ACL_ERROR;
        goto exit_config;
}

enum ACL_status ACL_check_client(ACL_t *acl, uint8_t *mac)
{
        return ACL_check_client_str(acl, uint8_array_to_mac(mac));
}

enum ACL_status ACL_check_client_str(ACL_t *acl, const char *mac)
{   
        if (!acl || !mac)
                return ACL_DENY;

        /* If ACL is disabled, allow every client */
        if (!acl->enabled)
                return ACL_ALLOW;

        char *value = NULL;
        bool found = false;

        llist_foreach(acl->entries, {
                value = (char *)node->data;

                if (strncmp(value, mac, 17) == 0) {
                        found = true;
                        break;
                }
        })

        if (found) {
                return (acl->is_blacklist) ? ACL_DENY : ACL_ALLOW;
        } else {
                return (acl->is_blacklist) ? ACL_ALLOW : ACL_DENY;
        }
}

void ACL_destroy(ACL_t **acl)
{
        if (!acl || !(*acl))
                return;
        
        ACL_t *pACL = *acl;

        llist_destroy(&pACL->entries);
        free(pACL);
        *acl = NULL;
}

