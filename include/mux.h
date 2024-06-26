#pragma once

#include <linux/if_packet.h>    /* tpacket_auxdata */
#include <stdint.h>             /* [u]int*_t       */
#include <stdio.h>              /* size_t          */
#include <pthread.h>            /* mutex           */

#include "circ_buf.h"           /* kernel ringbuff API */

#define MAX_PKT_SZ 1518

/* Total memory requirements: 
 * = 8 (NO_GATES) * PKTS_PER_GATE * 2048 (sizeof pkt_t) */
#define PKTS_PER_GATE (16384*8)   /* must be pow 2 */
#define RBUF_TOTAL_LEN (PKTS_PER_GATE * sizeof(pkt_t))
#define RBUF_MASK (RBUF_TOTAL_LEN - 1)


/* wrapper structure over anciliary data & pkt */
typedef struct {
    size_t  pkt_len;            /* length of pkt data */
    size_t  auxdata_len;        /* length of aux data */
    uint8_t pkt[MAX_PKT_SZ];    /* pkt data           */
    uint8_t auxdata[508];       /* aux data           */
} pkt_t;    /* 2048 bytes (must be power fo 2) */

/* gate ring buffers & mutexes */
extern struct circ_buf gate_rbuf[];
extern pthread_mutex_t gate_mutex[];

/* API */
ssize_t send_pkt(int32_t sockfd, pkt_t *pkt, struct sockaddr_ll *dst_addr);

ssize_t send_pkt_from_gate(int32_t sockfd, struct sockaddr_ll *dst_addr);

