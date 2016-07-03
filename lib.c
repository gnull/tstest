#include "internal.h"

int udp_socket(unsigned short port)
{
	int sock;
	int err;

	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock == -1)
		return perror("socket"), -errno;

	struct sockaddr_in addr = {
		.sin_family = AF_INET,
		.sin_port = htons(port),
		.sin_addr = { .s_addr = htonl(INADDR_ANY) }
	};
	err = bind(sock, (struct sockaddr *)&addr, sizeof(addr));
	if (err)
		return perror("bind"), -errno;

	return sock;
}

int packet_socket(const char *ifname)
{
	int sock;
	int err;

	sock = socket(AF_PACKET, SOCK_RAW, 0);
	if (sock == -1)
		return perror("socket"), -errno;

	struct sockaddr_ll addr = {
		.sll_family = AF_PACKET,
		.sll_ifindex = if_nametoindex(ifname),
		.sll_protocol = htons(ETH_P_IP),
	};
	err = bind(sock, (struct sockaddr *)&addr, sizeof(addr));
	if (err)
		return perror("bind"), -errno;

	return sock;
}

int setup_tstamp(int sock, int tsing_flags)
{
	int val, err, vallen = sizeof val;

	val = tsing_flags;
	printf("setting ts_flags: 0x%x\n", val);
	err = setsockopt(sock, SOL_SOCKET, SO_TIMESTAMPING, &val, vallen);
	if (err)
		return perror("setsockopt"), -errno;

	err = getsockopt(sock, SOL_SOCKET, SO_TIMESTAMPING, &val, &vallen);
	if (err)
		return perror("getsockopt"), -errno;

	printf("new ts_flags: 0x%x\n", val);

	return 0;
}

void tv2ts(const struct timeval *tv, struct timespec *ts)
{
	ts->tv_sec = tv->tv_sec;
	ts->tv_nsec = tv->tv_usec * 1000;
}

/* res = a - b; */
void ts_sub(volatile struct timespec *res,
	    const struct timespec *a, const struct timespec *b)
{
	typeof(res->tv_sec) sec;
	typeof(res->tv_nsec) nsec;

	sec = a->tv_sec - b->tv_sec;
	if (a->tv_nsec < b->tv_nsec) {
		nsec = nano + a->tv_nsec - b->tv_nsec;
		sec--;
	} else {
		nsec = a->tv_nsec - b->tv_nsec;
	}

	res->tv_sec = sec;
	res->tv_nsec = nsec;
}

static void stats_reserve(struct stats *ss)
{
	int n = ss->n;
	int res = ss->res;
	double *vect = ss->vect;

	if (!res) {
		res = 32;
		vect = malloc(res * sizeof(*vect));
	}

	if (n + 1 > res) {
		res <<= 1;
		vect = realloc(vect, res * sizeof(*vect));
	}

	ss->n = n;
	ss->res = res;
	ss->vect = vect;
}

void stats_push(struct stats *ss, const struct timespec *ts)
{
	stats_reserve(ss);

	ss->vect[ss->n++] = 1e+6 * ts->tv_sec + 1e-3 * ts->tv_nsec;
}

double stats_mean(const struct stats *ss)
{
	int i;
	double mean = 0;

	for (i = 0; i < ss->n; ++i)
		mean += ss->vect[i] / ss->n;

	return mean;
}

#define square(a) ((a) * (a))

double stats_dev(const struct stats *ss)
{
	int i;
	double dev = 0;

	double mean = stats_mean(ss);

	for (i = 0; i < ss->n; ++i)
		dev += square(mean - ss->vect[i]) / ss->n;

	dev = sqrt(dev);

	return dev;
}

void stats_print(char *str, const struct stats *ss)
{
	double dev, mean;

	printf("%s: packets %d: ", str, ss->n);

	if (!ss->n) {
		printf("(none)\n");
	} else {
		mean = stats_mean(ss);
		dev = stats_dev(ss);
		printf("%g +- %g microseconds\n", mean, dev);
	}
}
