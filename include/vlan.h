#pragma once

#include <stdint.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <linux/if_packet.h>
#include <net/ethernet.h> /* the L2 protocols */

#define ETH_VLAN_OFFSET  (6*2)
#define ETH_VLAN_TAG_TPID  0x8100
#define ETH_VLAN_TAG_SIZE  4

/** Ethernet 802.1q field structure */
struct eth_vlan_hdr {
	uint16_t tpid;
	union {
		struct {
			uint16_t vid : 12;
			uint16_t dei : 1;
			uint16_t pri : 3;
		};
		uint16_t tci;
	};
};


/**
 * Extract (return pointer to) the 802.1q tag from a packet.
 * Returns NULL if the packet has no 802.1q tag.
 */
static struct eth_vlan_hdr *packet_extract_dot1q(void *pkt_buf, ssize_t len)
{
	if ((len) < (ETH_VLAN_OFFSET + ETH_VLAN_TAG_SIZE))
		return 0;
	struct eth_vlan_hdr *vlan = (struct eth_vlan_hdr *)(pkt_buf + ETH_VLAN_OFFSET);
	if (ntohs(vlan->tpid) != ETH_VLAN_TAG_TPID)
		return 0;
	return vlan;
}

static int packet_build_vlan_iov(uint8_t *aux_buf, size_t *buf_len, 
								 const struct eth_vlan_hdr *vlan_hdr)
{
	*(uint16_t*)(aux_buf) = htons(vlan_hdr->tpid);
	*(uint16_t*)(aux_buf + 2) = htons(vlan_hdr->tci);
	*buf_len = ETH_VLAN_TAG_SIZE;
	return *buf_len;
}

