#ifndef UTILS_TD_H__
#define UTILS_TD_H__

#include <time.h>

typedef struct timespec TD_V;

#define TD_GET(v) clock_gettime(CLOCK_MONOTONIC, &v)

#define TD_CALC_s(e, s) ((double)e.tv_sec + 1.0e-9 * e.tv_nsec) - ((double)s.tv_sec + 1.0e-9 * s.tv_nsec)
#define TD_CALC_ms(e, s) (1000 * e.tv_sec + e.tv_nsec / 1000000) - (1000 * s.tv_sec + s.tv_nsec / 1000000)
#define TD_CALC_us(e, s) (1000000 * e.tv_sec + e.tv_nsec / 1000) - (1000000 * s.tv_sec + s.tv_nsec / 1000)
#define TD_CALC_ns(e, s) (1000000000 * e.tv_sec + e.tv_nsec) - (1000000000 * s.tv_sec + s.tv_nsec)

#endif // UTILS_TD_H__