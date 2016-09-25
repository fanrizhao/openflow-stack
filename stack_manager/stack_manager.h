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


#endif /*STACK_MANAGER_H_*/
