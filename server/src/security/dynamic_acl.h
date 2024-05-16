#ifndef __DYNAMIC_ACL__
#define __DYNAMIC_ACL__

#include "acl.h"
#include <stdint.h>
#include "../transaction_cache.h"

/**
 * Function checks for the number of transactions that are present in the transaction cache.
 * If half or more transactions have the same chaddr as the one provided, the client is 
 * added to the ACL list provided. 
 *
 * The acl is supposed to be a blacklist. Return ACL_status based on the criteria of 
 * a blacklist.
 *
 * If an error occurs, function always returns ACL_ALLOW
 */
enum ACL_status dynamic_ACL_check(ACL_t *acl, uint8_t *chaddr, transaction_cache_t *tcache);

/* Returns a new ACL structure that is already set to be a blacklist */
ACL_t *dynamic_ACL_new();

void dynamic_ACL_destroy(ACL_t **acl);

#endif // !__DYNAMIC_ACL__

