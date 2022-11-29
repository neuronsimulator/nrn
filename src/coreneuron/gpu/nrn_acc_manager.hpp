/*
# =============================================================================
# Copyright (c) 2016 - 2022 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================
*/
#pragma once

namespace coreneuron {
struct Memb_list;
struct NrnThread;
struct NetSendBuffer_t;
void setup_nrnthreads_on_device(NrnThread* threads, int nthreads);
void delete_nrnthreads_on_device(NrnThread* threads, int nthreads);
void update_nrnthreads_on_host(NrnThread* threads, int nthreads);

void update_net_receive_buffer(NrnThread* _nt);

// Called by NModl
void realloc_net_receive_buffer(NrnThread* nt, Memb_list* ml);
void update_net_send_buffer_on_host(NrnThread* nt, NetSendBuffer_t* nsb);

void update_weights_from_gpu(NrnThread* threads, int nthreads);
void init_gpu();
}  // namespace coreneuron
