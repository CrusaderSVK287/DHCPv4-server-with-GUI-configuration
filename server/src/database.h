#ifndef __DATABASE_H__
#define __DATABASE_H__

#include "dhcp_packet.h"
#include "transaction.h"
#include <stdint.h>

/* Store a dhcp message in permanent database storage */
int database_store_message(dhcp_message_t *message);

/* Load a database entry based on given xid and mac addresses */
transaction_t *database_load_transaction(uint32_t xid, uint8_t mac[6]);
transaction_t *database_load_transaction_str(uint32_t xid, const char *mac);

/* Function returns transaction based on given xid from permanent database */
transaction_t *database_load_transaction_xid(uint32_t xid);

/*
 * Function returns transaction from database based on the mac address given. 
 * WARNING: there may be more transactions with the same chaddr. Preferably, use 
 * database_load_transaction_xid or best, database_load_transaction
 */
transaction_t *database_load_transaction_mac(uint8_t mac[6]);
transaction_t *database_load_transaction_mac_str(const char *mac);

#endif // !__DATABASE_H__

