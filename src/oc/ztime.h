#pragma once

#include <time.h>

#define MAX_ZTIME        5
#define MAX_ZTIME_DETAIL 0xff
extern size_t ztime[MAX_ZTIME];
extern int ztcnt[MAX_ZTIME];
extern size_t ztime_detail[MAX_ZTIME][MAX_ZTIME_DETAIL + 1];

inline size_t zt_realtime() {
    timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    const size_t billion{1000000000};
    // const size_t floatmask{0Xfffffffffffff};  // 52 bits
    return ((billion * ts.tv_sec) + ts.tv_nsec);  // & floatmask;
}

#define ZTBEGIN size_t zt = zt_realtime()
#define ZTTOTAL(i)                                     \
    zt = zt_realtime() - zt;                           \
    ztime_detail[i][ztcnt[i] & MAX_ZTIME_DETAIL] = zt; \
    ztime[i] += zt;                                    \
    ++ztcnt[i];

inline double zttotal(int i) {
    return double(ztime[i]) / 1000000000.;
}
