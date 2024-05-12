#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <linux/if_packet.h>
#include <netinet/in.h>
#include <net/ethernet.h>
#include <net/if.h>

#include "util.h"

#define MAX_LEN 1518

void print_raw_hex(uint8_t *buf, ssize_t buflen)
{
	ssize_t i = 0, j;
	while (i < buflen) {
		for (j=0; (j < 16) && (i < buflen); j++) {
			printf("%02X ", buf[i]);
			i++;
		}
		printf("\n");
	}
}

int recv_packet(int sockfd, uint8_t *buf)
{
	int res = read(sockfd, buf, MAX_LEN);
	if (res < 0) {
		printf("read failed: %i\n", errno);
		exit(-1);
	}
	return res;
}

int main(int argc, char *argv[]) {
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

	struct sockaddr_ll addr;
	memset(&addr, 0x00, sizeof(addr));
	addr.sll_family = AF_PACKET;
	addr.sll_ifindex = intf.ifr_ifindex;
	res = bind(rawsock, (struct sockaddr *)&addr, sizeof(addr));
    DIE(res == -1, "unable to set bind iface (%s)", strerror(errno));

	while (1) {
		uint8_t buf[MAX_LEN];
		ssize_t plen = recv_packet(rawsock, buf);
        DEBUG("received packet of %ld bytes", plen);
		print_raw_hex(buf, plen);
	}
    return 0;
}

