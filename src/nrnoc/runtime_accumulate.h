#pragma once

#include <time.h>

extern int zt_accum_cnt;
extern size_t zt_accum_time;

inline size_t zt_accum_realtime() {
    timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    const size_t billion{1000000000};
    // const size_t floatmask{0Xfffffffffffff};  // 52 bits
    return ((billion * ts.tv_sec) + ts.tv_nsec);  // & floatmask;
}

#define ZT_ACCUMULATE_START size_t zt = zt_accum_realtime()
#define ZT_ACCUMULATE_PAUSE        \
    zt = zt_accum_realtime() - zt; \
    zt_accum_time += zt;           \
    ++zt_accum_cnt;

inline double zt_accum_total() {
    return double(zt_accum_time) / 1000000000.;
}
