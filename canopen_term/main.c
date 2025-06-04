#include "co_wrapper.h"
#include "sdo.h"
#include "slcan.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>

#define PORT_ID "VID_F055&PID_1337"

static uint8_t last_id = 127;

static void cb_co_hb_started(uint8_t node_id, uint8_t idx, void *priv) { printf("hb_started %d\n", node_id); }
static void cb_co_hb_to(uint8_t node_id, uint8_t idx, void *priv) { printf("hb to %d: %d\n", node_id, OD_PERSIST_COMM.x1016_consumerHeartbeatTime[idx] & 0xFFFF); }
static void cb_co_hb_rst(uint8_t node_id, uint8_t idx, void *priv) { printf("hb rst %d\n", node_id); }
static void cb_co_nmt_change(uint8_t node_id, uint8_t idx, CO_NMT_internalState_t NMTstate, void *priv)
{
	printf("nmt change %d: => %d\n", node_id, NMTstate);
	last_id = node_id;
}

static void sp_rx(sp_t *sp, const uint8_t *data, size_t len) { slcan_parse(((CO_t *)sp->priv)->CANmodule, data, len); }

static struct
{
	uint8_t rx_data[2048];
	uint32_t rx_cnt;
} term_rx_data[256] = {0};

#define CHK(x) \
	if((sts = x) != 0) printf("ERR %s: %s\n", #x, sp_err2_str(sts))

static int check_for_EOF(void)
{
	if(feof(stdin)) return 1;
	int c = getc(stdin);
	if(c == EOF) return 1;
	ungetc(c, stdin);
	return 0;
}

static void sp_err(sp_t *sp)
{
	printf("Serial Port Error!\n");
	exit(-1);
}

int main(int argc, char *argv[])
{
	printf("================\n");

	CO_t *co;
	int sts;
	sp_list_t list = {0};
	sp_t sp = {0};

	while(sp_enumerate(&list))
		if(strstr(list.info.port, "ttyS") == NULL) printf("\t%s#%s#%s\n", list.info.port, list.info.description, list.info.hardware_id);

	while(sp_enumerate(&list))
		if(strstr(list.info.hardware_id, PORT_ID) != NULL)
		{
			strcat(sp.port_name, list.info.port);
			printf("Using %s %s %s\n", list.info.port, list.info.description, list.info.hardware_id);
			sp_enumerate_finish(&list);
			break;
		}

	CHK(co_wrapper_init(&co, &sp));
	if(sts) goto FIN;

	for(uint32_t i = 0; i < 127; i++)
	{
		CO_HBconsumer_initCallbackNmtChanged(co->HBcons, i, co, cb_co_nmt_change);
		CO_HBconsumer_initCallbackHeartbeatStarted(co->HBcons, i, co, cb_co_hb_started);
		CO_HBconsumer_initCallbackTimeout(co->HBcons, i, co, cb_co_hb_to);
		CO_HBconsumer_initCallbackRemoteReset(co->HBcons, i, co, cb_co_hb_rst);
	}

	// CHK(sp_open(&sp, "COM6", sp_rx, co));
	// CHK(sp_open(&sp, "/dev/ttyS5", sp_rx, co));
	CHK(sp_open(&sp, 0, sp_rx, co));
	// CHK(sp_open_ext(&sp, 0, sp_rx, co, 115200, SP_BYTESIZE_8, SP_PARITY_NONE, SP_STOP_BITS_1, SP_FLOW_NONE));
	if(sts) goto FIN;

	sp.cb_err = sp_err;

	uint8_t tx_buf[1024];
	uint32_t tx_sz = 0;
	while(!check_for_EOF())
	{
		while((tx_buf[tx_sz++] = getchar()) != '\n' && tx_sz < sizeof(tx_buf))
			;
		tx_buf[tx_sz - 1] = '\0';
		co_term_master_request(&co->term, last_id, tx_buf, tx_sz);
		tx_sz = 0;
	}

FIN:
	printf("waiting port close...\n");

	sp_close(&sp);
	co_wrapper_deinit(&co);
	printf("END! exiting...\n");
	return 0;
}

void co_term_listener_rx(CO_term_t *t, uint8_t id, const uint8_t *data, uint32_t len)
{
	last_id = id;
	for(uint32_t i = 0; i < len; i++)
	{
		term_rx_data[id].rx_data[term_rx_data[id].rx_cnt++] = data[i];
		if(data[i] == '\0' || data[i] == '\n' || data[i] == '\r')
		{
			printf("[%d]%.*s", id, term_rx_data[id].rx_cnt, term_rx_data[id].rx_data);
			term_rx_data[id].rx_cnt = 0;
		}
		if(term_rx_data[id].rx_cnt >= sizeof(term_rx_data[id].rx_data)) term_rx_data[id].rx_cnt = 0;
	}
}
void co_term_process_rx_req(CO_term_t *t, const uint8_t *data, uint32_t len) {}