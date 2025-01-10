#include "sp.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

typedef struct timespec TD_V;

#define TD_GET(v) clock_gettime(CLOCK_MONOTONIC, &v)

#define TD_CALC_s(e, s) ((double)e.tv_sec + 1.0e-9 * e.tv_nsec) - ((double)s.tv_sec + 1.0e-9 * s.tv_nsec)
#define TD_CALC_ms(e, s) (1000 * e.tv_sec + e.tv_nsec / 1000000) - (1000 * s.tv_sec + s.tv_nsec / 1000000)
#define TD_CALC_us(e, s) (1000000 * e.tv_sec + e.tv_nsec / 1000) - (1000000 * s.tv_sec + s.tv_nsec / 1000)
#define TD_CALC_ns(e, s) (1000000000 * e.tv_sec + e.tv_nsec) - (1000000000 * s.tv_sec + s.tv_nsec)

const uint16_t vendor_id = 0xF055;
const uint16_t product_id = 0x1337;

#ifdef _WIN32
//  For Windows (32- and 64-bit)
#include <windows.h>
#define SLEEP_MS(msecs) Sleep(msecs)
#else
// #define _POSIX_C_SOURCE 199309L // or greater
#include <time.h>
#define SLEEP_MS(msecs)                   \
	do                                    \
	{                                     \
		struct timespec ts;               \
		ts.tv_sec = msecs / 1000;         \
		ts.tv_nsec = msecs % 1000 * 1000; \
		nanosleep(&ts, NULL);             \
	} while(0)
#endif

void rx(sp_t *sp, const uint8_t *data, size_t len)
{
	printf("\t%zu", len);
	for(uint32_t i = 0; i < len; i++)
	{
		put_ascii(data[i]);
	}
	putchar('\n');
}

#define CHK(x) \
	if((sts = x) != 0) printf("ERR %s: %s\n", #x, sp_err2_str(sts))

int main(void)
{
	int sts;
	sp_list_t list = {0};
	sp_t sp = {0};

	while(sp_enumerate(&list))
		printf("\t--> %s %s %s\n", list.info.port, list.info.description, list.info.hardware_id);

	CHK(sp_open(&sp, "COM6", rx, NULL));
	// CHK(sp_open(&sp, "/dev/ttyS5", rx, NULL));

#if 1
	CHK(sp_write(&sp, "L", 1));
	for(uint32_t i = 0; i < 100; i++)
	{
		// if(i == 95) sp_close(&sp);
		uint8_t dat[128] = "t77F17F";
		CHK(sp_write(&sp, dat, 7));
		SLEEP_MS(50);
	}
#endif

	CHK(sp_write(&sp, "O", 1));
	SLEEP_MS(2000);
	CHK(sp_write(&sp, "L", 1));
	SLEEP_MS(5000);
	// CHK(sp_write(&sp, "O", 1));
	printf("waiting port close...\n");
	sp_close(&sp);

	// TD_V st = {0}, fn = {0};

	// long long t = TD_CALC_ms(fn, st), tot_num = 0;
	// double tt = t * 0.001;
	// printf("Passed tot num: %lld bytes, time: %lld, baud %.6f Mbps\n", tot_num, t, tot_num * 0.000001 / tt);

	printf("END!\n");
	return 0;
}