#ifndef STACK_MANAGER_H_
#define STACK_MANAGER_H_

#include <regex.h>
#include <stdbool.h>
#include <stddef.h>
#include "list.h"
#include "packets.h"

struct secchan;

/* Behavior when the connection to the controller fails. */
enum fail_mode {
    FAIL_OPEN,                  /* Act as learning switch. */
    FAIL_CLOSED                 /* Drop all packets. */
};

/* Maximum number of management connection listeners. */
#define MAX_MGMT 8

#define MAX_CONTROLLERS 3

#define MAX_DATAPATHS 4

/* Settings that may be configured by the user. */
struct settings {
    /* Overall mode of operation. */
    bool discovery;           /* Discover the controller automatically? */
    bool in_band;             /* Connect to controller in-band? */

    /* Related vconns and network devices. */
    int num_datapaths;
    const char *dp_names[MAX_DATAPATHS];        /* Local datapath. */
    int num_controllers;        /* Number of configured controllers. */
    const char *controller_names[MAX_CONTROLLERS]; /* Controllers (if not discovery mode). */
    const char *listener_names[MAX_MGMT]; /* Listen for mgmt connections. */
    size_t n_listeners;          /* Number of mgmt connection listeners. */
    const char *monitor_name;   /* Listen for traffic monitor connections. */

    /* Failure behavior. */
    enum fail_mode fail_mode; /* Act as learning switch if no controller? */
    int max_idle;             /* Idle time for flows in fail-open mode. */
    int probe_interval;       /* # seconds idle before sending echo request. */
    int max_backoff;          /* Max # seconds between connection attempts. */

    /* Packet-in rate-limiting. */
    int rate_limit;           /* Tokens added to bucket per second. */
    int burst_limit;          /* Maximum number token bucket size. */

    /* Discovery behavior. */
    regex_t accept_controller_regex;  /* Controller vconns to accept. */
    const char *accept_controller_re; /* String version of regex. */
    bool update_resolv_conf;          /* Update /etc/resolv.conf? */

    /* Spanning tree protocol. */
    bool enable_stp;

    /* Emergency flow protection/restoration behavior. */
    bool emerg_flow;
};

struct half {
    struct rconn *rconn;
    struct ofpbuf *rxbuf;
    int n_txq;                  /* No. of packets queued for tx on 'rconn'. */
};

struct relay {
    struct list node;

#define HALF_LOCAL 0
#define HALF_REMOTE 1
    struct half halves[2];

    /* The secchan has a primary connection (relay) to an OpenFlow controller.
     * This primary connection actually makes two connections to the datapath:
     * one for OpenFlow requests and responses, and one that is only used for
     * receiving asynchronous events such as 'ofp_packet_in' events.  This
     * design keeps replies to OpenFlow requests from being dropped by the
     * kernel due to a flooded network device.
     *
     * The secchan may also have any number of secondary "management"
     * connections (relays).  These connections do not receive asychronous
     * events and thus have a null 'async_rconn'. */
    bool is_mgmt_conn;          /* Is this a management connection? */
    struct rconn *async_rconn;  /* For receiving asynchronous events. */
};

struct hook_class {
    bool (*local_packet_cb)(struct relay *, void *aux);
    bool (*remote_packet_cb)(struct relay *, void *aux);
    void (*periodic_cb)(void *aux);
    void (*wait_cb)(void *aux);
    void (*closing_cb)(struct relay *, void *aux);
};

void add_hook(struct secchan *, const struct hook_class *, void *);

struct ofp_packet_in *get_ofp_packet_in(struct relay *);
bool get_ofp_packet_eth_header(struct relay *, struct ofp_packet_in **,
                               struct eth_header **);
void get_ofp_packet_payload(struct ofp_packet_in *, struct ofpbuf *);


#endif /*STACK_MANAGER_H_*/
