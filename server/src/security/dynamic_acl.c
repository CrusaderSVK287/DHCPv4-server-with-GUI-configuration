#include "dynamic_acl.h"
#include "acl.h"
#include <cclog_macros.h>
#include <string.h>
#include <stdbool.h>
#include "../utils/xtoy.h"

enum ACL_status dynamic_ACL_check(ACL_t *acl, uint8_t *chaddr, transaction_cache_t *tcache)
{
        if (!acl || !chaddr || !tcache)
                return ACL_ALLOW;
        
        /* Check if the acl is enabled */
        if (!acl->enabled)
                return ACL_ALLOW;

        /* Check for the number of transactions assosiated with the client */
        int associated_transactions = 0;
        for (int i = 0; i < tcache->size; i++) {
                /* If the timer on transaction isnt running, its not a transaction in use */
                if (tcache->transactions[i]->timer->is_running == false) {
                        continue;
                }
                
                dhcp_message_t *m = trans_get_index(tcache->transactions[i], 0);
                if (!m) {
                        continue;
                }

                if (memcmp(chaddr, m->chaddr, 6) == 0) {
                        associated_transactions++;
                }
        }

        /* Add the new mac address to the ACL list */
        if (associated_transactions >= tcache->size / 2) {
                char *mac = strdup(uint8_array_to_mac(chaddr));
                llist_append(acl->entries, mac, true);
        }

        return ACL_check_client(acl, chaddr);
}

ACL_t *dynamic_ACL_new()
{
        ACL_t *acl = ACL_new();
        if_null(acl, error);

        acl->is_blacklist = true;

        return acl;
error:
        return NULL;
}

void dynamic_ACL_destroy(ACL_t **acl)
{
        ACL_destroy(acl);
}
