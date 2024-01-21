# DHCP-Server
Implementation of DHCP server in C with focus on configuration accessibility

# Restrictions
* Server assumes ALL dhcp messages received/send contain option 53 - DHCP message type. All messages not containing this option are DROPPED.

# Deviations from RFC-2131 standard
* While address allocation process in chapter 4.3.1 is respected and followed, the following step in allocation process is skipped for the time being.
  ```
        o The client's previous address as recorded in the client's (now
        expired or released) binding, if that address is in the server's
        pool of available addresses and not already allocated, ELSE
  ```
  The standard states that these steps SHOULD be followed, and we do not yet have API for clients history, this step is for now skipped.

