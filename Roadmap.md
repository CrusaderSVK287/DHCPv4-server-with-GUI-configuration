# General roadmap for expected path of development.
When criteria for a feature is made, commit hash will be added as a link to track progress.
When a set of features in category are made, pull request and commit hash will be added.
Tasks will not be worked on necessarily in order they are written. 
Tasks may be worked on even when flagged as done, flag just means that the feature was merged.
This roadmap may change in future

## Server:
* [Done - PR 966bd1d07aa8bf8c861643c3a206b0e51e035941] Make DORA handshake with multiple clients from pool:
    - Logging setup (CCLog) [Done - commit f82c0a834e15434ec4631010c01f8e079e3c191a]
    - Server logic [Done - commit b64510aa38ab2a2346b3ffd7048b0c18b67d02bd]
    - DHCP message parser and builder [Done - commit d661b10d5e470c9151353485ac3dda4a361db6fc]
    - Basic IP allocator API [Done - commit 190e4cc3f95ecebca981bb1a2a1635d7da70de7c]
* UNIX server and commands implementation for configuring:
    - UNIX server for ipc communication with gui
    - Command usage API (Maybe command execution could be made on separate thread if doesnt require server cooperation, e.g. changing config file without applying changes)
* DHCP configuration
    - IP allocator configuration for IP address pools
    - Extend IP allocator API
    - Configuration option for dhcp options.
* DHCP messages communication
    - Implement rest of DHCP message types handling
    - Implement cache for transactions to store for parameter checks
    - Bring all messages handlers up to RFC-2131 standard
* Transaction database
    - Transaction database structure
    - Database API to store, retrieve data etc.
* Other configuration
    - ACL configuration
    - Log verbosity configuration
    - Other general system configuration
* Security
    - ACL feature
    - DHCP starvation prevention
    - Active DHCP probing (Scanning network for rogue DHCP servers)

## GUI:
* UNIX client
    - UNIX client for communication with server
    - Commands API for sending commands to server (might be similiar to the server one since will use similiar syntax)
* curses UI
    - Root user authentification
    - Navigable menus/submenus for configuration
    - Emulated CLI with configuration commands
