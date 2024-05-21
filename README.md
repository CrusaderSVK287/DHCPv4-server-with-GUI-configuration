# DHCP-Server
Implementation of DHCP server in C with focus on configuration accessibility

# Restrictions
* Server assumes ALL dhcp messages received/send contain option 53 - DHCP message type. All messages not containing this option are DROPPED.
* Because of the Implementation, if client refuses assigned address for whatever reason, this address will be returned back 
  to the pool of available addresses. However, this takes some time (depending on configuration).
  There is a high chance of log message appearing, satating `Failed to retrieve lease of address <ip address>, possible missconfiguration`. 
  If in previous logs there is a dhcpdecline on this address or dhcpoffer offering this address but no dhcprequest, 
  it is likely that this case occured and the message can be ignored. Logging of this message does not alter 
  the servers flow.

# Deviations from RFC-2131 standard
* While address allocation process in chapter 4.3.1 is respected and followed, the following step in allocation process is skipped for the time being.
  ```
        o The client's previous address as recorded in the client's (now
        expired or released) binding, if that address is in the server's
        pool of available addresses and not already allocated, ELSE
  ```
  The standard states that these steps SHOULD be followed, and we do not yet have API for clients history, this step is for now skipped.

# Note

This repository was originally located on gitlab but was recently migrated to github.
As of now it is missing a proper README, documentation and similiar stuff. Until this 
note is here, consider the missing stuff as TODO.

