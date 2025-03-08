#pragma once

#include <nrnconf.h>
#if DARWIN

#include <os/log.h>
#include <os/signpost.h>
// Static logging variables at the top of hh.cpp
static os_log_t zzlog = os_log_create("com.neuron.hh", "tracing");
static os_signpost_id_t zzspid = os_signpost_id_generate(zzlog);

#define NRN_TRACE_START(name) os_signpost_interval_begin(zzlog, zzspid, name, "Start");
#define NRN_TRACE_END(name)   os_signpost_interval_end(zzlog, zzspid, name, "End");

#endif
