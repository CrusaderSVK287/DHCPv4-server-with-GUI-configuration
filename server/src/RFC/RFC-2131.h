#ifndef __RFC_2131_H__
#define __RFC_2131_H__

#define MAGIC_COOKIE 0x63825363

enum dhcp_message_type {
    DHCP_DISCOVER = 1,
    DHCP_OFFER = 2,
    DHCP_REQUEST = 3,
    DHCP_DECLINE = 4,
    DHCP_ACK = 5,
    DHCP_NAK = 6,
    DHCP_RELEASE = 7,
    DHCP_INFORM = 8
};

#endif // !__RFC_2131_H__

