#ifndef __THR_NOTIFIER_H__
#define __THR_NOTIFIER_H__

#include <condition_variable>
#include <mutex>
#include <thread>

typedef struct
{
	std::condition_variable cv;
	std::mutex mtx;
	int var = 0;
} thr_wait_t;

#define THR_WAIT(X)                               \
	{                                             \
		std::unique_lock<std::mutex> lk(X.mtx);   \
		X.cv.wait(lk, [] { return X.var == 1; }); \
	}

#define THR_NOTIFY(X)                          \
	{                                          \
		std::lock_guard<std::mutex> lk(X.mtx); \
		X.var = 1;                             \
	}                                          \
	X.cv.notify_all()

#endif // __THR_NOTIFIER_H__