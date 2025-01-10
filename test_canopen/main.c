#include "co_wrapper.h"
#include "sdo.h"
#include "slcan.h"
#include "timedate.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>

#define PORT_ID "VID_F055&PID_1337"

TS_V tstart, tdiff, tnow;

static void cb_co_nmt_change(uint8_t node_id, uint8_t idx, CO_NMT_internalState_t NMTstate, void *priv) { printf("nmt change %d: => %d\n", node_id, NMTstate); }
static void cb_co_hb_started(uint8_t node_id, uint8_t idx, void *priv) { printf("hb_started %d\n", node_id); }
static void cb_co_hb_to(uint8_t node_id, uint8_t idx, void *priv) { printf("hb to %d: %d\n", node_id, OD_PERSIST_COMM.x1016_consumerHeartbeatTime[idx] & 0xFFFF); }
static void cb_co_hb_rst(uint8_t node_id, uint8_t idx, void *priv) { printf("hb rst %d\n", node_id); }

void cb_co_frame_rx(void *priv, can_msg_t *msg)
{
	return;

	PRINT_TIME_MS(tstart);
	printf(" > %-4x  [%d]  ", msg->id.std, msg->DLC);
	for(uint32_t i = 0; i < msg->DLC; i++)
		printf(" %02X", msg->data[i]);
	putchar('\n');
}

void cb_co_frame_tx(void *priv, can_msg_t *msg)
{
	return;

	PRINT_TIME_MS(tstart);
	printf(" < %-4x  [%d]  ", msg->id.std, msg->DLC);
	for(uint32_t i = 0; i < msg->DLC; i++)
		printf(" %02X", msg->data[i]);
	putchar('\n');
}

static void sp_rx(sp_t *sp, const uint8_t *data, size_t len)
{
#ifdef PRINT_FRAMES
	PT();
	putchar('\t');
	for(uint32_t i = 0; i < len; i++)
	{
		put_ascii(data[i]);
	}
	putchar('\n');
#endif

	CO_t *co = (CO_t *)sp->priv;
	slcan_parse(co->CANmodule, data, len);
}

#define CHK(x) \
	if((sts = x) != 0) printf("ERR %s: %s\n", #x, sp_err2_str(sts))

int main(void)
{
	TS_GET(tstart);

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
	if(sts) goto FIN;

#if 0
	CHK(sp_write(&sp, "L", 1));
	for(uint32_t i = 0; i < 100; i++)
	{
		// if(i == 95) sp_close(&sp);
		uint8_t dat[128] = "t77F17F";
		CHK(sp_write(&sp, dat, 7));
		SLEEP_MS(50);
	}
#endif

	CHK(sp_write(&sp, "Z0\r", 3));
	// CHK(sp_write(&sp, "O", 1));
	// SLEEP_MS(2000);
	// CHK(sp_write(&sp, "L", 1));
	// SLEEP_MS(5000);
	// // CHK(sp_write(&sp, "O", 1));

	// SLEEP_MS(200);

	for(uint32_t i = 0; i < 2; i++)
	{
		uint16_t pht = 20;
		sts = write_SDO(co->SDOclient, 127, 0x1017, 0, (uint8_t *)&pht, sizeof(pht), 2000);
		if(sts)
		{
			PRINT_TIME_MS(tstart);
			printf(": HB time SDO: x%x\n", sts);
			if(i == 1) goto FIN;
		}
		else
			break;
	}

	size_t rs = 0;
	uint8_t data[256];

	PRINT_TIME_MS(tstart);
	printf(": ==================================\n\tread SDO: start\n");
	TS_GET(tdiff);
	for(uint32_t i = 0, succ = 0; i < 3000; i++, succ++)
	{
		sts = read_SDO(co->SDOclient, 127, 0x1008, 0, data, sizeof(data), &rs, 2000);
		// sts = read_SDO(co->SDOclient, 127, 0x1016, 0, data, sizeof(data), &rs, 2000);
		if(sts == 0)
		{
			if((succ % 300) == 0 && succ)
			{
				printf("Diff:");
				PRINT_TIME_MS(tdiff);
				tdiff = tnow;

				printf(" ms\tCount:%d SDO ->", succ);
				for(uint32_t j = 0; j < rs; j++)
					put_ascii(data[j]);
				putchar('\n');
			}
		}
		else
		{
			PRINT_TIME_MS(tstart);
			printf(": read SDO: x%x: %zu (%d times succ)\n", sts, rs, succ);
			break;
		}
	}

	uint16_t pht = 800;
FIN:
	write_SDO(co->SDOclient, 127, 0x1017, 0, (uint8_t *)&pht, sizeof(pht), 200);

	printf("waiting port close...\n");

	sp_close(&sp);
	co_wrapper_deinit(&co);
	printf("END! exiting...\n");
	return 0;
}