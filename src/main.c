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

#include "slice.h"
#include "mux.h"
#include "util.h"
#include "vlan.h"

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

typedef struct {
    int32_t            sockfd;
    struct sockaddr_ll *dst_addr;
} thread_data_t;

int recv_packet(int sockfd, uint8_t *buf, size_t *buflen, struct eth_vlan_hdr *vlan_hdr) {
    struct iovec iov[1];
    struct sockaddr_ll sll;
    struct msghdr msg;
    struct cmsghdr *cmsg;
	union {
		struct cmsghdr	cmsg;
		char		buf[CMSG_SPACE(sizeof(struct tpacket_auxdata))];
	} cmsg_buf;

    msg.msg_name        = &sll;
    msg.msg_namelen     = sizeof(sll);
    msg.msg_iov         = iov;
    msg.msg_iovlen      = 1;
    msg.msg_control     = &cmsg_buf;
    msg.msg_controllen  = sizeof(cmsg_buf);
    msg.msg_flags       = 0;
    iov[0].iov_base = buf;
    iov[0].iov_len = *buflen;
    int res = recvmsg(sockfd, &msg, 0);
    RET(res == -1, -1, "recvmsg failed (%s)", strerror(errno));
	for (cmsg = CMSG_FIRSTHDR(&msg); cmsg; cmsg = CMSG_NXTHDR(&msg, cmsg)) {
		struct tpacket_auxdata *aux;
		size_t len;
		struct eth_vlan_hdr *tag;
		if (cmsg->cmsg_len < CMSG_LEN(sizeof(struct tpacket_auxdata)) ||
			cmsg->cmsg_level != SOL_PACKET ||
			cmsg->cmsg_type != PACKET_AUXDATA) {
			/* This isn't a PACKET_AUXDATA auxiliary data item */
			// DEBUG("ign cmsg %02lX", cmsg->cmsg_len);
			continue;
		}
		aux = (struct tpacket_auxdata *)CMSG_DATA(cmsg);
		// if (!VLAN_VALID(aux, aux)) {
		// 	continue;
		// }
		// DEBUG("aux vlan %03hX %02hX", aux->tp_vlan_tpid, aux->tp_vlan_tci);
		vlan_hdr->tpid = aux->tp_vlan_tpid;
		vlan_hdr->tci = aux->tp_vlan_tci;
	}
    *buflen = res;
    return res;
}

void *send_worker(void *data)
{
    thread_data_t *td = data;

    /* sender main loop */
    while (1) {
        send_pkt_from_gate(td->sockfd, td->dst_addr);
    }

    return NULL;
}

int main(int argc, char *argv[]) {
    uint8_t buf[MAX_PKT_SZ];

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

    /* capture ancilarry data (for VLAN tag) */
    int optval = 1;
    res = setsockopt(rawsock, SOL_PACKET, PACKET_AUXDATA, &optval, sizeof(optval));
    DIE(res < 0, "unable to setsockopt PACKET_AUXDATA (%s)", strerror(errno));

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

    /* create sender thread */
    pthread_t tsend;
    thread_data_t td = { .sockfd = rawsock, .dst_addr = &addr };
    res = pthread_create(&tsend, NULL, send_worker, &td);
    DIE(res, "unable to create sender thread (%s)", strerror(res));

    /* TODO: niflo */

    return 0;

    /* dead */
    while (1) {
        struct eth_vlan_hdr vlan_hdr;
        size_t plen = sizeof(buf);
        res = recv_packet(rawsock, buf, &plen, &vlan_hdr);
        if (res < 0)
            continue;

        /* ignore outgoing packet */
        if (memcmp(&buf[6], macreq.ifr_hwaddr.sa_data, 6) == 0)
            continue;

        DEBUG("received packet of %ld bytes", plen);
        if (vlan_hdr.tpid == ETH_VLAN_TAG_TPID) {
            INFO("Has vlan tag: p %i, vlan id %i\n", vlan_hdr.pri, vlan_hdr.vid);
        }
        print_raw_hex(buf, plen);

        pkt_t pkt = {
            .pkt_len = plen,
        };
        memcpy(pkt.pkt, buf, plen);
        /* build ancillary data with VLAN tag */
        vlan_hdr.tpid = ETH_VLAN_TAG_TPID;
        // vlan_hdr.tci = 0;
        vlan_hdr.pri = 3;
        vlan_hdr.vid = 10;
        if (vlan_hdr.tpid == ETH_VLAN_TAG_TPID) {
            pkt.auxdata_len = sizeof(pkt.auxdata);
            packet_build_vlan_iov(pkt.auxdata, &pkt.auxdata_len, &vlan_hdr);
            DEBUG("SEND DATA:");
            print_raw_hex(pkt.auxdata, pkt.auxdata_len);
        }
        res = send_pkt(rawsock, &pkt, &addr);
        DIE(res == -1, "unable to send packet (%s)", strerror(errno));
    }

    return 0;
}

