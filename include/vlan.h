#include <stdint.h>
#include <arpa/inet.h>

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

