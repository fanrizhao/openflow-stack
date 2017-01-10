#ifndef STACK_MANAGER_H_
#define STACK_MANAGER_H_

#include <regex.h>
#include <stdbool.h>
#include <stddef.h>
#include "list.h"
#include "packets.h"

struct stDp_conn {
	struct rconn *rconn;
	struct ofpbuf *txbuf;
	struct ofpbuf *rxbuf;
	unsigned int txcnt;
	unsigned int rxcnt;
	};

struct stMm {
	struct pvconn *pvconn;
	struct stDp_conn *secchan;
	struct stDp_conn *dp[4];
	int dp_num;
	};
	
/* Settings that may be configured by the user. */
struct settings {
	/* Overall mode of operation. */
	bool discovery;				/* Discover the controller automatically? */
	bool in_band;				/* Connect to controller in-band? */
	
	/* Related vconns and network devices. */
	const char *dp_names[4];		/* Local datapath. */
	int num_datapath;			/* Number of datapath. */
	const char *sc_name;			/* Controllers (if not discovery mode). */
	
	/* Failure behavior. */
	enum fail_mode fail_mode;		/* Act as learning switch if no controller? */
	int max_idle;				/* Idle time for flows in fail-open mode. */
	int probe_interval;			/* # seconds idle before sending echo request. */
	int max_backoff;			/* Max # seconds between connection attempts. */
	
	/* Packet-in rate-limiting. */
	int rate_limit;				/* Tokens added to bucket per second. */
	int burst_limit;			/* Maximum number token bucket size. */	
};

#endif /*STACK_MANAGER_H_*/
