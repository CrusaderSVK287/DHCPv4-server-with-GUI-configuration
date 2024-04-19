#ifndef __DATABASE_HPP__
#define __DATABASE_HPP__

#include <string>
#include <vector>
#include <stdint.h>

#define DB_FILE_PATH_FORMAT "/var/dhcp/database/%08x_%02x%02x%02x%02x%02x%02x.dhcp"
#define METADATA_FORMAT "T:%08lxCA:%02x%02x%02x%02x%02x%02xP:"
#define METADATA_LEN 128

struct dhcp_message {
    uint8_t opcode;
    uint8_t htype;
    uint8_t hlen;
    uint8_t hops;

    uint32_t xid;

    uint16_t secs;
    uint16_t flags;

    uint32_t ciaddr;
    uint32_t yiaddr;
    uint32_t siaddr;
    uint32_t giaddr;
    uint8_t chaddr[16];

    char sname[64];
    char filename[128];

    uint32_t cookie;
    uint8_t options[336];

    uint32_t time_when_stored;
    char msg_type[32];
};

std::vector<dhcp_message> database_load_entry(std::string path);

#endif // !__DATABASE_HPP__

