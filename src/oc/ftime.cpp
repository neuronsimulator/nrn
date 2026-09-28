#include "oc_ansi.h"

#include <chrono>

static double start_time = 0.;

double nrn_time() {
    // Get nanoseconds converted in seconds
    const auto now = std::chrono::time_point_cast<std::chrono::duration<double>>(
        std::chrono::system_clock::now());
    start_time = now.time_since_epoch().count();
    return start_time;
}

void hoc_startsw() {
    hoc_ret();
    hoc_pushx(nrn_time());
}

void hoc_stopsw() {
    double now = nrn_time();
    hoc_ret();
    hoc_pushx(now - start_time);
    start_time = now;
}

double nrn_timeus() {
    const auto now = std::chrono::time_point_cast<std::chrono::duration<double>>(
        std::chrono::system_clock::now());
    return now.time_since_epoch().count();
}
