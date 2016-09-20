
#include <config.h>
#include "stack_manager.h"
#include <assert.h>
#include <errno.h>
#include <getopt.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>

#include "command-line.h"
#include "compiler.h"
#include "daemon.h"
#include "dirs.h"
#include "fault.h"
#include "leak-checker.h"
#include "list.h"
#include "ofpbuf.h"
#include "openflow/openflow.h"
#include "packets.h"
#include "poll-loop.h"
#include "rconn.h"
#include "timeval.h"
#include "util.h"
#include "vconn-ssl.h"
#include "vconn.h"
#include "vlog-socket.h"

#include "vlog.h"
#define THIS_MODULE VLM_stack_manager

static void parse_options(int argc, char *argv[]);
static void usage(void) NO_RETURN;

int
main(int argc, char *argv[])
{
	struct rconn *r_dp0 = NULL;
	struct rconn *r_dp1 = NULL;
	struct rconn *p_secchan = NULL;
	struct pvconn *pvconn = NULL;
	struct ofpbuf *buf_dp0 = NULL;
	struct ofpbuf *buf_dp1 = NULL;
	struct ofpbuf *buf_secchan = NULL;
	unsigned int p_rx = 0;
	unsigned int p_tx = 0;
	unsigned int a_rx = 0;
	unsigned int a_tx = 0;
	
	set_program_name(argv[0]);
	register_fault_handlers();
	
	time_init();
	vlog_init();
	parse_options(argc, argv);
	signal(SIGPIPE, SIG_IGN);
	die_if_already_running();
	daemonize();
	 
	{
		int retval;
		retval = pvconn_open("ptcp:6632", &pvconn);
		if (!retval || retval == EAGAIN) {
			printf("succesive open pvconn\n");
        	} else {
			printf("fault\n");
		}
		printf("retval = %d\n", retval);
	}
	{
		r_dp0 = rconn_create(0, 0);
		r_dp1 = rconn_create(0, 0);
		/*
		 *  dp0 local_board0, dp1 board1
		 */
		rconn_connect(r_dp0, "tcp:127.0.0.1:6631");
		rconn_connect(r_dp1, "tcp:192.168.1.15:6631");
	}
	
	while(1){
		int retval;
		unsigned int i;
		
		{
			int retval;
			struct vconn *new_vconn = NULL;
			retval = pvconn_accept(pvconn, OFP_VERSION, &new_vconn);
			if(!retval) {
				p_secchan = rconn_new_from_vconn("passive", new_vconn);
				printf("accepted\n");
			}
		}
		rconn_run(r_dp0);
		rconn_run(r_dp1);
		if(p_secchan) {
			rconn_run(p_secchan);
			if (rconn_is_connected(r_dp0) && rconn_is_connected(r_dp1) && rconn_is_connected(p_secchan)) {
				for(i = 0; i < 50; i++) {
					if(buf_dp0 == NULL) {
						buf_dp0 = rconn_recv(r_dp0);
					}
					if(buf_dp0 != NULL) {
						printf("active recive %d'th packet\n",++a_rx);
						retval = rconn_send(p_secchan, buf_dp0, NULL);
						if(!retval) {
							printf("passive transmit %d'th packet\n",++p_tx);
							buf_dp0 = NULL;
						}
					}
					
					if(buf_dp1 == NULL) {
                                                buf_dp1 = rconn_recv(r_dp1);
                                        }
                                        if(buf_dp1 != NULL) {
                                                printf("active recive %d'th packet\n",++a_rx);
                                                retval = rconn_send(p_secchan, buf_dp1, NULL);
                                                if(!retval) {
                                                        printf("passive transmit %d'th packet\n",++p_tx);
                                                        buf_dp1 = NULL;
                                                }
                                        }

					if(buf_secchan == NULL) {
						buf_secchan = rconn_recv(p_secchan);
					}
					if(buf_secchan != NULL){
						printf("passive recive %d'th packet\n",++p_rx);
						retval = rconn_send(r_dp0, buf_secchan, NULL);
						if(!retval) {
							printf("active transmit %d'th packet\n",++a_tx);
							buf_secchan = NULL;
						}
						
						retval = rconn_send(r_dp1, buf_secchan, NULL);
                                                if(!retval) {
                                                        printf("active transmit %d'th packet\n",++a_tx);
                                                        buf_secchan = NULL;
                                                }
					}
				}
			}
			rconn_run_wait(p_secchan);
			rconn_recv_wait(p_secchan);
		}
		rconn_run_wait(r_dp0);
		rconn_run_wait(r_dp1);
		rconn_recv_wait(r_dp0);
                rconn_recv_wait(r_dp1);
		pvconn_wait(pvconn);
		poll_block();
	}
    return 0;
}

static void
parse_options(int argc, char *argv[])
{
    enum {
        OPT_ACCEPT_VCONN = UCHAR_MAX + 1,
        OPT_BOOTSTRAP_CA_CERT,
        VLOG_OPTION_ENUMS,
        LEAK_CHECKER_OPTION_ENUMS
    };
    static struct option long_options[] = {
        {"accept-vconn", required_argument, 0, OPT_ACCEPT_VCONN},
        {"verbose",     optional_argument, 0, 'v'},
        {"help",        no_argument, 0, 'h'},
        {"version",     no_argument, 0, 'V'},
        DAEMON_LONG_OPTIONS,
        VLOG_LONG_OPTIONS,
        LEAK_CHECKER_LONG_OPTIONS,
#ifdef HAVE_OPENSSL
        VCONN_SSL_LONG_OPTIONS
        {"bootstrap-ca-cert", required_argument, 0, OPT_BOOTSTRAP_CA_CERT},
#endif
        {0, 0, 0, 0},
    };
    char *short_options = long_options_to_short_options(long_options);
    for (;;) {
        int c;

        c = getopt_long(argc, argv, short_options, long_options, NULL);
        if (c == -1) {
            break;
        }

        switch (c) {
        case 'h':
            usage();

        case 'V':
            printf("%s %s compiled "__DATE__" "__TIME__"\n",
                   program_name, VERSION BUILDNR);
            exit(EXIT_SUCCESS);

        DAEMON_OPTION_HANDLERS

        VLOG_OPTION_HANDLERS

        LEAK_CHECKER_OPTION_HANDLERS

#ifdef HAVE_OPENSSL
        VCONN_SSL_OPTION_HANDLERS

        case OPT_BOOTSTRAP_CA_CERT:
            vconn_ssl_set_ca_cert_file(optarg, true);
            break;
#endif

        case '?':
            exit(EXIT_FAILURE);

        default:
            abort();
        }
    }
    free(short_options);
    argc -= optind;
    argv += optind;
/*    if (argc < 1 || argc > 2) {
        ofp_fatal(0, "need one or two non-option arguments; "
                  "use --help for usage");
    }*/
}

static void
usage(void)
{
    printf("%s: secure channel, a relay for OpenFlow messages.\n"
           "usage: %s [OPTIONS] DATAPATH [CONTROLLER]\n"
           "DATAPATH is an active connection method to a local datapath.\n"
           "CONTROLLER is an active OpenFlow connection method; if it is\n"
           "omitted, then secchan performs controller discovery.\n",
           program_name, program_name);
    vconn_usage(true, true, true);
    printf(" "
           );
    daemon_usage();
    vlog_usage();
    printf("\nOther options:\n"
           "  -h, --help              display this help message\n"
           "  -V, --version           display version information\n");
    leak_checker_usage();
    exit(EXIT_SUCCESS);
}
