#include <sys/socket.h>
#include <netpacket/packet.h>
#include <net/ethernet.h> /* the L2 protocols */
#include <netinet/in.h>

#include <linux/net_tstamp.h>
#include <linux/sockios.h>

#include <sys/ioctl.h>
#include <net/if.h>

#include <linux/errqueue.h>

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <assert.h>
#include <math.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <linux/version.h>

#define datalen 1500
#define controllen 200

#define for_cmsg(i, msg) for (i = CMSG_FIRSTHDR(&msg); i; i = CMSG_NXTHDR(&msg, i))

#define nano (1000*1000*1000)

#define ERR(a, ...) fprintf(stderr, a "\n", ##__VA_ARGS__)

#define ts_empty(ts) (!(ts)->tv_sec && !(ts)->tv_nsec)

/* Ибо в linux/errqueue.h нет на ядре 3.2 вместо struct
 * scm_timestamping использовали struct timespec[3].
 */
struct scm_timestamping {
        struct timespec ts[3];
};

struct stats {
	double *vect;
	int n;
	int res;
};

void stat_push(struct stats *ss, const struct timespec *ts);
void stat_print(char *str, const struct stats *ss);


int setup_tstamp(int sock, int tsing_flags);

void ts_sub(volatile struct timespec *res,
	    const struct timespec *a, const struct timespec *b);

void stats_push(struct stats *ss, const struct timespec *ts);


int udp_socket(unsigned short port);

int packet_socket(const char *ifname);

void stats_print(char *str, const struct stats *ss);
