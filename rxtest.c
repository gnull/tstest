#include <time.h>

#include "internal.h"

static struct stats stat_user, stat_soft;

static void usage(char *name)
{
	printf("Usage:\n\t"
	       "%s [udp <port> | packet <iface>] <count>\n", name);
}

static int setup_rx_tstamp(int sock)
{
	int tsing_flags = SOF_TIMESTAMPING_RX_SOFTWARE |
		SOF_TIMESTAMPING_SOFTWARE |
		SOF_TIMESTAMPING_RAW_HARDWARE;

	return setup_tstamp(sock, tsing_flags);
}

static void rx_tstamp(int sock)
{
	char data[datalen], control[controllen];
	struct cmsghdr *i;
	struct timespec ts, *soft, *hard;
	int err;

	struct iovec iov = {
		.iov_base = data,
		.iov_len = sizeof(data)
	};
	struct msghdr msg = {
		.msg_iov = &iov,
		.msg_iovlen = 1,
		.msg_control = control,
		.msg_controllen = sizeof(control)
	};
	err = recvmsg(sock, &msg, 0);
	if (err < 0)
		return perror("recvmsg");
	err = clock_gettime(CLOCK_REALTIME, &ts);
	if (err)
		perror("gettimeofday");

	struct scm_timestamping *tss = NULL;

	for_cmsg(i, msg) {
		if (i->cmsg_level != SOL_SOCKET ||
			i->cmsg_type != SCM_TIMESTAMPING)
			continue;

		tss = (struct scm_timestamping *) CMSG_DATA(i);
		break;
	}

	if (!tss) {
		ERR("SCM_TIMESTAMPING not found!");
		return;
	}

	soft = tss->ts;
	hard = tss->ts + 2;

	if (!ts_empty(soft)) {
		ts_sub(&ts, &ts, soft);
		stats_push(&stat_user, &ts);

		if (!ts_empty(hard)) {
			ts_sub(soft, soft, hard);
			stats_push(&stat_soft, soft);
		}
	}
}

int main(int argc, char **argv)
{
	int sock;
	int err;
	int pktnum;
	int i;
	struct timespec mean;
	char *ifname;

	if (argc < 2)
		return usage(argv[0]), 1;

	if (!strcmp(argv[1], "udp")) {
		if (argc < 4)
			return usage(argv[0]), 1;
		int port = atoi(argv[2]);
		pktnum = atoi(argv[3]);
		sock = udp_socket(port);
	} else if (!strcmp(argv[1], "packet")) {
		if (argc < 4)
			return usage(argv[0]), 1;
		ifname = argv[2];
		pktnum = atoi(argv[3]);
		sock = packet_socket(ifname);
	} else {
		return usage(argv[0]), 1;
	}

	if (sock < 0)
		return 1;

	err = setup_rx_tstamp(sock);
	if (err)
		return err;

	for (i = 0; i < pktnum; ++i)
		rx_tstamp(sock);

	stats_print("hard->soft delay", &stat_soft);
	stats_print("soft->user delay", &stat_user);

	return 0;
}
