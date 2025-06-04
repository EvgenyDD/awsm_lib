#include "../percent_tracker.h"
#include "co_wrapper.h"
#include "sdo.h"
#include "slcan.h"
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>

#define PORT_ID "VID_F055&PID_1337"

static void sp_rx(sp_t *sp, const uint8_t *data, size_t len) { slcan_parse(((CO_t *)sp->priv)->CANmodule, data, len); }

#define CHK(x) \
	if((sts = x) != 0) printf("ERR %s: %s\n", #x, sp_err2_str(sts))

static void print_data(const uint8_t *data, uint32_t sz)
{
	if(sz == 0) return;
	if(sz == 1)
	{
		if((int8_t)data[0] < 0)
			printf("{8} %u/%d ", (uint8_t)data[0], (int8_t)data[0]);
		else
			printf("{8} %u ", (uint8_t)data[0]);
	}
	if(sz == 2)
	{
		uint16_t v;
		int16_t vs;
		memcpy(&v, data, sz);
		memcpy(&vs, data, sz);
		if(vs < 0)
			printf("{16} %u/%d ", v, vs);
		else
			printf("{16} %u ", v);
	}
	if(sz == 4)
	{
		uint32_t v;
		int32_t vs;
		float vf;
		memcpy(&v, data, sz);
		memcpy(&vs, data, sz);
		memcpy(&vf, data, sz);
		if(vs < 0)
			printf("{32} %u/%d ", v, vs);
		else
			printf("{32} %u ", v);
		if(v != 0)
			printf("{F} %f ", vf);
		else
			printf("{F} 0 ");
	}
	if(sz == 8)
	{
		uint64_t v;
		int64_t vs;
		double vd;
		memcpy(&v, data, sz);
		memcpy(&vs, data, sz);
		memcpy(&vd, data, sz);
		if(vs < 0)
			printf("{64} %llu/%lld ", v, vs);
		else
			printf("{64} %llu ", v);
		if(v != 0)
			printf("{D} %f ", vd);
		else
			printf("{D} 0 ");
	}
	if(sz >= 1)
	{
		printf("{S} %.*s ", sz, data);
	}
}

static void print_data_hex(const uint8_t *data, uint32_t sz)
{
	if(sz == 0) return;
	if(sz == 1)
	{
		if(data[0] > 9) printf("{8} x%X ", data[0]);
	}
	if(sz == 2)
	{
		uint16_t v;
		memcpy(&v, data, sz);
		if(v > 9) printf("{16} x%X ", v);
	}
	if(sz == 4)
	{
		uint32_t v;
		memcpy(&v, data, sz);
		if(v > 9) printf("{32} x%X ", v);
	}
	if(sz == 8)
	{
		uint64_t v;
		memcpy(&v, data, sz);
		if(v > 9) printf("{64} x%llX ", v);
	}
}

int main(int argc, char *argv[])
{
	if(argc != 2)
	{
		fprintf(stderr, "Error! Wrong argument count!\nUsage:\n"
						"  id      - device CAN ID\n");
		return -1;
	}

	CO_t *co;
	int sts;
	sp_list_t list = {0};
	sp_t sp = {0};
	progress_tracker_t tr;

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
	CHK(sp_open(&sp, 0, sp_rx, co));
	if(sts) goto FIN;

	uint8_t id = atoi(argv[1]);
	printf("===== SDO table for device ID: %d =====\n", id);
	typedef struct
	{
		uint32_t error;
		size_t readed_size;
		uint8_t storage[256];
	} entry_t;
	uint8_t *mem = malloc(65536 * 256 * sizeof(entry_t));
	memset(mem, 0xFF, 65536 * 256 * sizeof(entry_t));

	entry_t *e = (entry_t *)mem;

	PERCENT_TRACKER_INIT(tr);
	char el[128], est[128];
	for(uint32_t idx = 0; idx <= 0xFFFF; idx++)
	{
		for(uint32_t sub = 0; sub < 256; sub++)
		{
			PERCENT_TRACKER_TRACK(tr, (double)(idx * 256 + sub) / (double)(65536 * 256),
								  { tr.time_ms_pass > 60000 ? sprintf(el, "%lld min %lld sec", tr.time_ms_pass / 60000, (tr.time_ms_pass / 1000) % 60) : sprintf(el, "%lld sec", tr.time_ms_pass / 1000);
								 	tr.time_ms_est > 60000 ? sprintf(est, "%lld min %lld sec", tr.time_ms_est / 60000, (tr.time_ms_est / 1000) % 60) : sprintf(est, "%lld sec", tr.time_ms_est / 1000); 
									fprintf(stderr, "\rinfo:  x%04x x%x ...  %.1f%% | pass: %s | est: %s        ",
											idx, sub, 100.0 * tr.progress, el, est); });
			e[idx * 256 + sub].readed_size = sizeof(e[idx * 256 + sub].storage);
			e[idx * 256 + sub].error = read_SDO(co->SDOclient, id, idx, sub, e[idx * 256 + sub].storage, sizeof(e[idx * 256 + sub].storage),
												&e[idx * 256 + sub].readed_size, 100);
			e[idx * 256 + sub].storage[e[idx * 256 + sub].readed_size] = 0;
			if(e[idx * 256 + sub].error == CO_SDO_AB_NOT_EXIST) break;
			if(e[idx * 256 + sub].error == CO_SDO_AB_SUB_UNKNOWN) break;
		}
	}
	printf("\n");
	{
		for(uint32_t idx = 0; idx <= 0xFFFF; idx++)
		{
			bool idx_printed = false;
			for(uint32_t sub = 0; sub < 256; sub++)
			{
				if(e[idx * 256 + sub].error == 0)
				{
					if(idx_printed)
						printf("       : 0x%x [%2lld ]: ", sub, e[idx * 256 + sub].readed_size);
					else
						printf("0x%04X : 0x%x [%2lld ]: ", idx, sub, e[idx * 256 + sub].readed_size);
					print_data(e[idx * 256 + sub].storage, e[idx * 256 + sub].readed_size);
					print_data_hex(e[idx * 256 + sub].storage, e[idx * 256 + sub].readed_size);
					printf("\n");
					idx_printed = true;
				}
			}
		}
	}

	free(mem);

FIN:
	printf("waiting port close...\n");

	sp_close(&sp);
	co_wrapper_deinit(&co);
	printf("END! exiting...\n");
	return 0;
}
