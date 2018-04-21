#include <arpa/inet.h>
#include <errno.h>
#include <net/if.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <poll.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define MULTICAST_ADDR "ff02::262d"
#define MULTICAST_PORT 1818
//#define MULTICAST_PORT 12
//#define INTERFACE_INDEX 3

__attribute__((noreturn))
static void pdie(const char *msg) {
    perror(msg);
    exit(1);
}

static void toNetID(char *peer) {
    char *end = strchr(peer, '%');
    if (end == NULL) {
        end = peer + strlen(peer);
    }

    if (end < peer + 4)
        return;

    memmove(peer, end - 4, 4);
    peer[4] = '\0';
}

static void join_mcast(int sock) {
    struct ipv6_mreq mreq = {
        .ipv6mr_multiaddr = {{{0}}}
    };

    if (1 != inet_pton(AF_INET6, MULTICAST_ADDR, &mreq.ipv6mr_multiaddr))
        pdie("inet_pton failed");

    struct if_nameindex *nameindex = if_nameindex();
    if (!nameindex)
        pdie("if_nameindex()");

    unsigned joined = 0;
    for (unsigned i = 0; nameindex[i].if_index != 0; ++i) {
        mreq.ipv6mr_interface = nameindex[i].if_index;

        // Joining will fail on interfaces that don't have IPv6
        if (setsockopt(sock, IPPROTO_IPV6, IPV6_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) == 0)
            ++joined;
    }

    fprintf(stderr, "Joined multicast group on %u interfaces\n", joined);
    if (joined == 0) {
        fprintf(stderr, "Failed to join the multicast group on any interfaces");
        exit(EXIT_FAILURE);
    }

    if_freenameindex(nameindex);
}

static void handle_error(const struct msghdr *msg) {
    char remoteaddr[INET6_ADDRSTRLEN];

    if (getnameinfo(msg->msg_name, msg->msg_namelen,
                remoteaddr, sizeof(remoteaddr), NULL, 0,
                NI_DGRAM | NI_NUMERICHOST | NI_NUMERICSERV) != 0) {
        static const char unknown[] = "(unknown)";
        memcpy(remoteaddr, unknown, sizeof(unknown));
    }

    fprintf(stderr, "%s was unreachable\n", remoteaddr);
}

static void handle_message(const char buf[], ssize_t len, const char *sender) {
    if (len < 5) {
        fprintf(stderr, "Received short packet, ignoring");
        return;
    }

    if (buf[0] != '\0') {
        printf("%s sent a packet with unhandled version %hhu\n", sender, buf[0]);
        return;
    }

    static const char MESG[] = {'M', 'E', 'S', 'G'};

    if (memcmp(buf + 1, MESG, sizeof(MESG)) == 0) {
        printf("%s: %.*s\n", sender, (int)(len - 5), buf + 5);
    } else {
        printf("%s sent unrecognized message type %4s\n", sender, buf + 1);
    }
}

int main(void) {
    int s = socket(AF_INET6, SOCK_DGRAM, 0);

    if (-1 == s)
        pdie("socket");

    static const int yes = 1;
    if (setsockopt(s, IPPROTO_IPV6, IPV6_RECVERR, &yes, sizeof(yes)) != 0)
        perror("setsockopt IPV6_RECVERR, true");

    if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) != 0)
        pdie("SO_REUSEADDR");

    struct sockaddr_in6 sin6 = {0};
    sin6.sin6_family = AF_INET6;
    sin6.sin6_port = htons(MULTICAST_PORT);
    sin6.sin6_addr = in6addr_any;
    //sin6.sin6_scope_id = INTERFACE_INDEX;
    sin6.sin6_scope_id = 0; // any

    if (-1 == bind(s, (struct sockaddr*)&sin6, sizeof(sin6)))
        pdie("bind");

    join_mcast(s);

    struct pollfd fds[1] = {
        {
            .fd = s,
            .events = POLLIN,
            .revents = 0
        }
    };

    while (poll(fds, 1, -1) > 0) {
        // max payload of UDP-on-IPv6-on-Ethernet
        // Jumbo layer 2 protocols (or IPv4) could be higher but honestly…
        char buf[1452];
        const bool err = fds[0].revents & POLLERR;

        struct sockaddr_storage srcaddr;
        socklen_t srcaddrlen = sizeof(srcaddr);

        if (!err) {
            ssize_t len = recvfrom(s, buf, sizeof(buf), 0, (struct sockaddr*)&srcaddr, &srcaddrlen);

            if (len == -1) {
                perror("recvfrom failed");
                break;
            }

            char sender[INET6_ADDRSTRLEN];

            if (getnameinfo((const struct sockaddr*)&srcaddr, srcaddrlen, sender, sizeof(sender),
                    NULL, 0, NI_DGRAM | NI_NUMERICHOST | NI_NUMERICSERV) != 0) {
                static const char unknown[] = "(unknown)";
                memcpy(sender, unknown, sizeof(unknown));
            }

            toNetID(sender);
            handle_message(buf, len, sender);
        } else {
            struct msghdr msg;
            memset(&msg, '\0', sizeof(msg));
            struct sockaddr_in6 sin6;
            msg.msg_name = &sin6;
            msg.msg_namelen = sizeof(sin6);

            if (recvmsg(s, &msg, MSG_ERRQUEUE) == -1) {
                perror("recvmsg");
                break;
            }

            handle_error(&msg);
        }
    }

    if (-1 == close(s))
        pdie("close");

    return 0;
}
