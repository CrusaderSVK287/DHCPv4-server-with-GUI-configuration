#include "RFC-2131.h"

const char* rfc2131_dhcp_message_type_to_str(enum dhcp_message_type t)
{
        switch (t) {
                case DHCP_DISCOVER: return "DHCP_DISCOVER";
                case DHCP_OFFER:    return "DHCP_OFFER";
                case DHCP_REQUEST:  return "DHCP_REQUEST";
                case DHCP_DECLINE:  return "DHCP_DECLINE";
                case DHCP_ACK:      return "DHCP_ACK";
                case DHCP_NAK:      return "DHCP_NAK"; 
                case DHCP_RELEASE:  return "DHCP_RELEASE";
                case DHCP_INFORM:   return "DHCP_INFORM";
                default:            return "Unknown DHCP Message Type";
        }
}

