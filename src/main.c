#include <arpa/inet.h>
#include <errno.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <net/ethernet.h>
#include <net/if.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

#include "util.h"

#define MAX_LEN 1518

void print_raw_hex(uint8_t *buf, ssize_t buflen) {
    ssize_t i = 0, j;
    while (i < buflen) {
        for (j = 0; (j < 16) && (i < buflen); j++) {
            printf("%02X ", buf[i]);
            i++;
        }
        printf("\n");
    }
}

int recv_packet(int sockfd, uint8_t *buf) {
    int res = read(sockfd, buf, MAX_LEN);
    RET(res == -1, -1, "unable to read pkt (%s)", strerror(errno));

    return res;
}

int main(int argc, char *argv[]) {
    uint8_t buf[MAX_LEN];

    /* cli args sanity check */
    DIE(argc < 2, "Usage: ./tsn <IFACE>");

    int rawsock = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    DIE(rawsock == -1, "unable to open raw socket (%s)", strerror(errno));

    struct ifreq intf;
    strcpy(intf.ifr_name, argv[1]);
    int res = ioctl(rawsock, SIOCGIFINDEX, &intf);
    DIE(res == -1, "SIOCGIFINDEX ioctl failed (%s)", strerror(errno));

    /* place NIC in promiscuous mode */
    struct ifreq ethreq;
    strncpy(ethreq.ifr_name, argv[1], IF_NAMESIZE);
    res = ioctl(rawsock, SIOCGIFFLAGS, &ethreq);
    DIE(res == -1, "unable to get NIC settings (%s)", strerror(errno));

    ethreq.ifr_flags |= IFF_PROMISC;
    res = ioctl(rawsock, SIOCSIFFLAGS, &ethreq);
    DIE(res == -1, "unable to set NIC settings (%s)", strerror(errno));

    /* determine MAC address of iface */
    struct ifreq macreq;
    strncpy(macreq.ifr_name, argv[1], IF_NAMESIZE);
    res = ioctl(rawsock, SIOCGIFHWADDR, &macreq);
    DIE(res == -1, "unable to get NIC MAC addr (%s)", strerror(errno));

    INFO("MAC addr: %02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx",
         macreq.ifr_hwaddr.sa_data[0], macreq.ifr_hwaddr.sa_data[1],
         macreq.ifr_hwaddr.sa_data[2], macreq.ifr_hwaddr.sa_data[3],
         macreq.ifr_hwaddr.sa_data[4], macreq.ifr_hwaddr.sa_data[5]);


    struct sockaddr_ll addr;
    memset(&addr, 0x00, sizeof(addr));
    addr.sll_family = AF_PACKET;
    addr.sll_ifindex = intf.ifr_ifindex;
    res = bind(rawsock, (struct sockaddr *)&addr, sizeof(addr));
    DIE(res == -1, "unable to set bind iface (%s)", strerror(errno));

    while (1) {
        ssize_t plen = recv_packet(rawsock, buf);
        if (plen == -1)
            continue;

        /* ignore outgoing packet */
        if (memcmp(&buf[6], macreq.ifr_hwaddr.sa_data, 6) == 0)
            continue;

        DEBUG("received packet of %ld bytes", plen);
        print_raw_hex(buf, plen);
    }

    return 0;
}

