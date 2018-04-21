#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>

#define IPV4 0

#define MCAST_ADDR "ff02::262d"
#define MCAST_PORT 1818
#define INTERFACE 16
#define MSGLEN 14

static int send_mesg(int s, const char *txt) {
    size_t len = strlen(txt);
    size_t msglen = 5 + len;
    char msg[msglen];
    memcpy(msg, "\x00MESG", 5);
    memcpy(msg + 5, txt, len);

    int err = send(s, msg, msglen, 0);
    if (err)
        perror("sendto");

    return err;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <message>\n", argv[0]);
        return 1;
    }

    int s = socket(AF_INET6, SOCK_DGRAM, 0);

    struct sockaddr_in6 sin6 = {0};
#if IPV4
    struct sockaddr_in sin4 = {0};
#endif

    int err;

    sin6.sin6_port = htons(MCAST_PORT);
    sin6.sin6_family = AF_INET6;
    sin6.sin6_scope_id = INTERFACE; // interface index (netdevice(7))
    if (inet_pton(AF_INET6, MCAST_ADDR, &sin6.sin6_addr) != 1) {
        fprintf(stderr, "inet_pton failed");
        return 7;
    }

#if IPV4
    int s4 = socket(AF_INET, SOCK_DGRAM, 0);
    sin4.sin_family = AF_INET; // turns out to be optional
    sin4.sin_port = htons(1234);
    if (inet_pton(AF_INET, "225.0.0.37", &sin4.sin_addr.s_addr) != 1) {
        fprintf(stderr, "inet_pton failed for ipv4");
        return 9;
    }
#endif

    err = connect(s, (struct sockaddr*)&sin6, sizeof(sin6));
    if (err) {
        perror("connect");
        return 1;
    }

#if 0
    if (strncasecmp(argv[1], "/nick ", 6) == 0)
        return send_nick(s, argv[1]);
    else
#endif
        return send_mesg(s, argv[1]);
}
