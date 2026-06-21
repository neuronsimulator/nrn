#pragma once

#include <cstddef>
#include <vector>

struct Memb_list;
struct NrnThread;

namespace neuron::gpu {

struct NetSendBuffer_t {
    int* _sendtype = nullptr;
    int* _vdata_index = nullptr;
    int* _pnt_index = nullptr;
    int* _weight_index = nullptr;
    double* _nsb_t = nullptr;
    double* _nsb_flag = nullptr;
    int _cnt = 0;
    int _size = 0;
    int reallocated = 0;

    explicit NetSendBuffer_t(int size);
    ~NetSendBuffer_t();

    void grow();

    [[nodiscard]] std::size_t size_of_object() const;
};

void net_send_buffer_ensure(Memb_list* ml);
void update_net_send_buffer_on_host(NrnThread* nt, NetSendBuffer_t* nsb);
void deliver_net_send_buffer_events(NrnThread* nt, NetSendBuffer_t* nsb);
void ensure_thread_net_send_buffers(NrnThread* nt);
void flush_mechanism_net_send_buffers(NrnThread* nt);

extern std::vector<int> net_buf_send_types;

}  // namespace neuron::gpu

extern "C" void hoc_register_net_send_buffering(int type);