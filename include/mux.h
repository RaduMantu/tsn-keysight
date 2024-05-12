#pragma once

#include <linux/if_packet.h>    /* tpacket_auxdata */
#include <stdint.h>             /* [u]int*_t       */
#include <stdio.h>              /* size_t          */
#include <pthread.h>            /* mutex           */

#include "circ_buf.h"           /* kernel ringbuff API */

#define MAX_PKT_SZ 1518

/* wrapper structure over anciliary data & pkt */
typedef struct {
    size_t  pkt_len;            /* length of pkt data */
    size_t  auxdata_len;        /* length of aux data */
    uint8_t pkt[MAX_PKT_SZ];    /* pkt data           */
    uint8_t auxdata[516];       /* aux data           */
} pkt_t;    /* 2048 bytes (must be power fo 2) */

/* gate ring buffers & mutexes */
extern struct circ_buf gate_rbuf[];
extern pthread_mutex_t gate_mutex[];

/* API */
ssize_t send_pkt(int32_t sockfd, struct sockaddr_ll *dst_addr);
