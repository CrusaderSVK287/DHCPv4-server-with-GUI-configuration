#include "database.hpp"
#include <cmath>
#include <cstddef>
#include <cstring>
#include <netinet/in.h>
#include <string>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include "logger.hpp"

#define MAGIC_COOKIE 0x63825363

static void get_msg_type(dhcp_message *m)
{
    if (!m) {
        log("Received NULL pointer to message");
        return;
    }

    memset(m->msg_type, 0, 32);

    int i = 0;
    while(i < 336) {
        if (m->options[i] == 0) {
            i++;
            continue;
        }
        if (m->options[i] == 255)
            break;
        
        if (m->options[i] == 53 && i + 2 < 336) {
            //handle dhcp message type
            switch (m->options[i + 2]) {
                case 1: strcpy( m->msg_type, "DISCOVER"); break;
                case 2: strcpy( m->msg_type, "OFFER");    break;
                case 3: strcpy( m->msg_type, "REQUEST");  break;
                case 4: strcpy( m->msg_type, "DECLINE");  break;
                case 5: strcpy( m->msg_type, "ACK");      break;
                case 6: strcpy( m->msg_type, "NAK");      break;
                case 7: strcpy( m->msg_type, "RELEASE");  break;
                case 8: strcpy( m->msg_type, "INFORM");   break;
                default:
                    strcpy(m->msg_type, "UNKNOWN");
                    break;
            }
        }

        i++;
        if (i >= 336) break;
        i += m->options[i] + 1;
        if (i >= 336) break;
    }

    if (strlen(m->msg_type) == 0) {
        strcpy(m->msg_type, "UNKNOWN");
    }
}

std::vector<dhcp_message> database_load_entry(std::string path)
{
    std::vector<dhcp_message> transaction;

    dhcp_message msg_buffer = {0};
    char header_buff[METADATA_LEN + 1];
    memset(header_buff, 0, METADATA_LEN + 1);

    int fd = open(path.c_str(), O_RDONLY);
    if (fd < 0) {
        log("Failed to open database entry file " + path);
        return transaction; // will be empty
    }

    size_t rv = 0;
    do {
        if ((rv = read(fd, header_buff, METADATA_LEN)) != METADATA_LEN) {
            if (rv == 0)
                break;

            log("Corupted databasa entry " + path + ", failed to load metadata"); 
            break;
        }

        /* -36 because 4 bytes for parsed time and 32 bytes for msg_type which is not in database */
        if (read(fd, &msg_buffer, 576) != 576) {
            log("Corupted database entry " + path + ", failed to load data"); 
            break;
        }

        if (sscanf(header_buff, "T:%08lx", (unsigned long *)&msg_buffer.time_when_stored) != 1) {
            log("Corupted database entry " + path + ", failed to parse metadata"); 
            continue;
        }

        msg_buffer.xid = ntohl(msg_buffer.xid);
        msg_buffer.secs = ntohs(msg_buffer.secs);
        msg_buffer.flags = ntohs(msg_buffer.flags);

        msg_buffer.ciaddr = ntohl(msg_buffer.ciaddr);
        msg_buffer.yiaddr = ntohl(msg_buffer.yiaddr);
        msg_buffer.siaddr = ntohl(msg_buffer.siaddr);
        msg_buffer.giaddr = ntohl(msg_buffer.giaddr);

        msg_buffer.cookie = ntohl(msg_buffer.cookie);

        if (msg_buffer.cookie != MAGIC_COOKIE)
            continue;

        get_msg_type(&msg_buffer);
        transaction.push_back(msg_buffer);
    } while (true);

    close(fd);

    return transaction;
}

