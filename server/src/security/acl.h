#ifndef __ACL_H__
#define __ACL_H__

#include "../utils/llist.h"
#include <stdbool.h>
#include <stdint.h>

enum ACL_status {
    ACL_ERROR   = -1,
    ACL_OK      = 0,
    ACL_ALLOW   = 1,
    ACL_DENY    = 2,
};

// TODO: When doing config interface, it may be possible to update the ACL without restating server
typedef struct acces_control_list {
    llist_t *entries;
    bool enabled;
    bool is_blacklist;
} ACL_t;

ACL_t* ACL_new();

/* Function loads acl entries from file provided by path. */
int ACL_load_acl_entries(ACL_t *acl, const char *path);

/* 
 * Function checks presence of client in the access controll list.
 * Returns eighter ACL_ALLOW, to allow client to be served or ACL_DENY
 * to deny client to be served. On any critical error, returns ACL_DENY 
 */
enum ACL_status ACL_check_client(ACL_t *acl, uint8_t *mac);
enum ACL_status ACL_check_client_str(ACL_t *acl, const char *mac);

void ACL_destroy(ACL_t **acl);

#endif // !__ACL_H__

