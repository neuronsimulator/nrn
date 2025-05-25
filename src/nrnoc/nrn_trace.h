#pragma once

#include <nrnconf.h>


#if DARWIN

#include <os/log.h>
#include <os/signpost.h>
// Static logging variables at the top of hh.cpp
static os_log_t zzlog;
static os_signpost_id_t zzspid;

static void nrn_trace_logging() {
    zzlog = os_log_create("com.neuron.hh", "tracing");
    zzspid = os_signpost_id_generate(zzlog);
}

#define NRN_TRACE_START(name) os_signpost_interval_begin(zzlog, zzspid, name, "Start");
#define NRN_TRACE_END(name)   os_signpost_interval_end(zzlog, zzspid, name, "End");

#else  // generic

#include <chrono>
#include <unordered_map>
#include <string>
#include <stdio.h>

// Portable tracing with std::chrono

FILE* temp_trace = fopen("temp.trace", "w");

#define NRN_TRACE_START(name) auto _start = std::chrono::steady_clock::now();

#define NRN_TRACE_END(name)                                                                      \
    do {                                                                                         \
        auto _end = std::chrono::steady_clock::now();                                            \
        auto _dur = std::chrono::duration_cast<std::chrono::nanoseconds>(_end - _start).count(); \
        fprintf(temp_trace, "%s: %ld ns\n", name, _dur);                                         \
    } while (0)

#endif  // generic
