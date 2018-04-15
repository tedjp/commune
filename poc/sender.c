#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>

#define IPV4 0

#define MCAST_ADDR "ff02::114"
#define MCAST_PORT 1818

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
    sin6.sin6_scope_id = 2; // interface index (netdevice(7))
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

    size_t len = strlen(argv[1]);
    size_t msglen = 5 + len;
    char msg[msglen];
    memcpy(msg, "\x00MESG", 5);
    memcpy(msg + 5, argv[1], len);

#if IPV4
    err = sendto(s4, msg, msglen, 0, (struct sockaddr*)&sin4, sizeof(sin4));
#else
    err = connect(s, (struct sockaddr*)&sin6, sizeof(sin6));
    if (err) {
        perror("connect");
        return 1;
    }
    //err = sendto(s, msg, sizeof(msg), 0, (struct sockaddr*)&sin6, sizeof(sin6));
    err = send(s, msg, msglen, 0);
#endif
    if (err) {
        perror("sendto");
        return 8;
    }

    return 0;
}
