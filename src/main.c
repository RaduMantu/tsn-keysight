#include <sys/socket.h>         /* socket    */
#include <sys/ioctl.h>          /* ioctl     */
#include <net/if.h>             /* ifreq     */
#include <net/ethernet.h>       /* ETH_P_ALL */
#include <arpa/inet.h>          /* htons     */

#include "util.h"

int32_t
main(int32_t argc, char *argv[])
{
    struct ifreq ethreq;
    int32_t      sockfd;
    int32_t      ans;
    ssize_t      rb;

    uint8_t      buff[0x1000];

    /* cli args sanity check */
    DIE(argc < 2, "Usage: ./tsn <iface>");

    /* open raw socket */
    sockfd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    DIE(sockfd == -1, "unable to open raw socket (%s)", strerror(errno));

    /* bind only to given iface */
    ans = setsockopt(sockfd, SOL_SOCKET, SO_BINDTODEVICE,
                     argv[1], strlen(argv[1]) + 1);
    DIE(ans == -1, "unable to set bind iface (%s)", strerror(errno));

    /* place NIC in promiscuous mode */
    strncpy(ethreq.ifr_name, argv[1], IF_NAMESIZE);
    ans = ioctl(sockfd, SIOCGIFFLAGS, &ethreq);
    DIE(ans == -1, "unable to get NIC settings (%s)", strerror(errno));

    ethreq.ifr_flags |= IFF_PROMISC;
    ans = ioctl(sockfd, SIOCSIFFLAGS, &ethreq);
    DIE(ans == -1, "unable to set NIC settings (%s)", strerror(errno));

    /* main loop */
    while(1) {
        /* read one packet */
        rb = recv(sockfd, buff, sizeof(buff), 0);
        DIE(rb == -1, "unable to recv from raw socket (%s)", strerror(errno));

        /* debug */
        DEBUG("received packet of %ld bytes", rb);

    }

    return 0;
}

