#include "dr.h"
#include "thr_notifier.h"
#include <atomic>
#include <thread>

thr_wait_t thr_wait[3];
std::atomic_int sss = 0;
std::atomic_int finished = 0;

const size_t cnt = 1000;

void thr()
{
	tx_t t = dr_tx_push("", "temp");
	dr_rx_push_sync("", "temp", [](const void *data, size_t size)
					{ sss++; });

	finished++;
	if(finished == cnt) THR_NOTIFY(thr_wait[2]);

	THR_WAIT(thr_wait[0])

	dr_tx(t, "123", 1);
	finished++;
	if(finished == cnt) THR_NOTIFY(thr_wait[2]);
	THR_WAIT(thr_wait[1]);
}

void test_threads()
{
	std::list<std::thread> thrds;

	for(uint32_t i = 0; i < cnt; i++)
	{
		thrds.emplace_back(thr);
	}

	THR_WAIT(thr_wait[2]);

	printf("!MIDDLE!\n");

	THR_NOTIFY(thr_wait[0]);

	THR_WAIT(thr_wait[2]);

	THR_NOTIFY(thr_wait[1]);

	for(auto it = thrds.begin(); it != thrds.end(); it++)
	{
		it->join();
	}
	printf("%d\n", (int)sss);
}

void test_1(void)
{
	printf("=== Test 1 ===\n");
	rx_t r = dr_rx_push_sync("", "string", [](const void *data, size_t size) {});
	tx_t t = dr_tx_push("PORT" + std::to_string(123), "string");
	rx_t rr = dr_rx_push_sync("PORT123", "string", [](const void *data, size_t size) {});
	tx_t tt = dr_tx_push("EMT", "string");
	dr_tx_pop(t);
	dr_tx_pop(tt);
	dr_rx_pop(r);
	dr_rx_pop(rr);
}

int main(void)
{
	printf("--- DR Test ---\n");
	printf("compile: %ld\n", __cplusplus);

#if 1
	rx_t _r = dr_rx_push_async("", "string", 128);
	rx_t r = dr_rx_push_sync("", "string", [](const void *data, size_t size) { /*printf(">> %lld %s\n", size, (char *)data); */ });

	tx_t t = dr_tx_push("PORT" + std::to_string(123), "string");
	std::string a = "42";
	// dr_rx_set(_r, false);
	rx_t rr = dr_rx_push_sync("PORT123", "string", [](const void *data, size_t size) { /*printf("No %lld %s\n", size, (char *)data);*/ });

	tx_t tt = dr_tx_push("EMT", "string");

	for(size_t i = 0; i < 30; i++)
	{
		a += "5555";
		if(i == 29) a = "wtf";

		int code;
		if((code = dr_tx(t, a.c_str(), a.length() + 1)) <= 0)
		{
			printf("Can't TX! %d\n", code);
		}
		if(i > 5)
		{
			const void *data;
			size_t size;
			while((size = dr_is_avail(_r, &data)) != 0)
			{
				printf("=>%s\n", (char *)data);
				dr_consume(_r);
			}
		}
		printf("*\n");
	}

	dr_rx_pop(r);
	dr_rx_pop(rr);
	dr_tx_pop(t);
	dr_tx_pop(tt);
#endif

	test_threads();

	// test_1();

	// std::thread t1([&]()
	// 			   { dr_type_t *tx_type_str2 = dr_new_type("string2"); });

	// dr_type_t *tx_type_str = dr_new_type("string");
	// dr_type_t *tx_type_str2 = dr_new_type("string2");
	// dr_del_type("string2");
	// // dr_del_type("strings");
	// dr_type_t *tx_type_str3 = dr_new_type("string3");
	// dr_type_t *tx_type_str8 = dr_new_type("string4");

	// dr_new_tx("string4");

	// // int rx_ch_str = dr_new_rx(tx_type_str, 0, );
	// // int rx_ch_str2 = dr_new_rx("string", 0, );

	// // int tx_chnl_str1 = dr_new_chnl(tx_type_str, "0");
	// t1.join();

	printf("All done!\n");
	return 0;
}