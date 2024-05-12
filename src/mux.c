#include <sys/socket.h>     /* msghdr, sendmsg */

#include "mux.h"
#include "slice.h"
#include "util.h"


#define PKTS_PER_GATE 512   /* must be pow 2 */
#define RBUF_MASK (PKTS_PER_GATE * sizeof(pkt_t) - 1)

/* per-gate ring buffers */
struct circ_buf gate_rbuf[NO_GATES];
pthread_mutex_t gate_mutex[NO_GATES];


/* send_pkt - sends a packet if any available for current gate
 *  sockfd    : output raw socket file descriptor
 *  @dst_addr : destination address
 *
 * NOTE: call this repeatedly from a sender thread:
 */
ssize_t send_pkt(int32_t sockfd, struct sockaddr_ll *dst_addr)
{
    size_t        gate;             /* current gate       */
    struct msghdr msg = { 0 };      /* message            */
    struct iovec  iov;              /* sparse data source */
    pkt_t         *pkt;             /* packet wrapper     */
    ssize_t       res = 0;          /* result             */

    /* get current gate */
    gate = get_current_gate();

    /* acquire lock for current gate (but don't block) */
    res = pthread_mutex_trylock(&gate_mutex[gate]);
    if (res)
        return 0;

    /* quit early if no message available in queue */
    if (gate_rbuf[gate].head == gate_rbuf[gate].tail)
        goto out;

    /* prepare message */
    pkt = (pkt_t *) (gate_rbuf[gate].buf + gate_rbuf[gate].tail);

    iov.iov_base = pkt->pkt;
    iov.iov_len  = pkt->pkt_len;

    msg.msg_name       = dst_addr;
    msg.msg_namelen    = sizeof(struct sockaddr_ll);
    msg.msg_iov        = &iov;
    msg.msg_iovlen     = 1;
    msg.msg_control    = &pkt->auxdata;
    msg.msg_controllen = sizeof(pkt->auxdata);

    /* try to send message */
    res = sendmsg(sockfd, &msg, 0);
    GOTO(res == -1, out, "unable to send msg (%s)", strerror(errno));

    /* consume structure from ringbuffer */
    gate_rbuf[gate].tail += sizeof(pkt_t);
    gate_rbuf[gate].tail &= RBUF_MASK;

out:
    /* release gate lock */
    pthread_mutex_unlock(&gate_mutex[gate]);

    return res;
}


/* _mux_init - ctor; intializes ring buffer related stuff
 *
 * NOTE: circ_buf's backing buffer size must be power of 2
 */
static void __attribute__((constructor))
_mux_init(void)
{
    int32_t res;    /* result */

    for (size_t i = 0; i < NO_GATES; i++) {
        /* allocate ring buffer */
        gate_rbuf[i].buf = calloc(PKTS_PER_GATE, sizeof(pkt_t));
        DIE(!gate_rbuf[i].buf, "unable to allocate ring buffer (%s)",
            strerror(errno));

        /* initialize mutex associated to ringbuffer */
        res = pthread_mutex_init(&gate_mutex[i], NULL);
        DIE(res, "unable to initalize mutex (%s)", strerror(res));
    }
}
