#include "neuron/gpu/config.hpp"
#include "neuron/gpu/net_receive_buffer.hpp"
#include "neuron/gpu/offload.hpp"

#include "multicore.h"
#include "nrnoc_ml.h"

#include <catch2/catch_test_macros.hpp>

#if defined(NRN_ENABLE_GPU) && defined(_OPENACC)
#include <openacc.h>
#endif

using namespace neuron::gpu;

// Standalone unit test: product update_net_receive_buffer reads this global.
// Without a definition the link leaves a null/broken reloc and SIGSEGVs.
short* nrn_is_artificial_ = nullptr;

namespace {
int g_stub_receive_calls = 0;

void stub_net_buf_receive(NrnThread* /*nt*/) {
    ++g_stub_receive_calls;
}
}  // namespace

TEST_CASE("net_receive_buffer registry and host enqueue", "[gpu][net_receive]") {
#if !defined(NRN_ENABLE_GPU)
    SKIP("NRN_ENABLE_GPU required");
#else
    net_buf_receive.clear();
    hoc_register_net_receive_buffering(stub_net_buf_receive, 42);
    REQUIRE(net_buf_receive.size() == 1);
    REQUIRE(net_buf_receive.front().second == 42);

    // Heap Memb_list without delete: this TU is OpenACC-instrumented and
    // ~Memb_list mishandles _net_receive_buffer / _net_send_buffer fields.
    auto* const ml = new Memb_list{};
    ml->nodecount = 4;
    net_receive_buffer_ensure(ml);
    REQUIRE(ml->_net_receive_buffer != nullptr);
    REQUIRE(ml->_net_receive_buffer->_size >= 4);

    NrnThread nt{};
    nt._t = 1.025;
    REQUIRE(net_receive_buffer_enqueue(&nt, ml, 3, 7, 1.0));
    REQUIRE(ml->_net_receive_buffer->_cnt == 1);
    REQUIRE(ml->_net_receive_buffer->_pnt_index[0] == 3);
    REQUIRE(ml->_net_receive_buffer->_weight_index[0] == 7);
    REQUIRE(ml->_net_receive_buffer->_nrb_flag[0] == 1.0);

    detail::net_receive_buffer_order(ml->_net_receive_buffer);
    REQUIRE(ml->_net_receive_buffer->_displ_cnt == 1);
    REQUIRE(ml->_net_receive_buffer->_nrb_index[0] == 0);

    free_net_receive_buffer(ml);
    REQUIRE(ml->_net_receive_buffer == nullptr);
#endif
}

TEST_CASE("net_receive_buffer upload exposes queued events on device", "[gpu][net_receive]") {
#if !defined(NRN_ENABLE_GPU) || !defined(_OPENACC)
    SKIP("NRN_ENABLE_GPU with OpenACC required");
#else
    if (acc_get_num_devices(acc_device_nvidia) < 1) {
        SKIP("No NVIDIA GPU available");
    }
    acc_init(acc_device_nvidia);
    acc_set_device_num(0, acc_device_nvidia);

    detail::reset_config_for_testing();
    detail::set_enable_for_testing(true);
    detail::set_backend_for_testing(Backend::Native);

    net_buf_receive.clear();
    hoc_register_net_receive_buffering(stub_net_buf_receive, 9);

    auto* const ml = new Memb_list{};
    ml->nodecount = 2;
    net_receive_buffer_ensure(ml);

    NrnThread nt{};
    nt.id = 0;
    nt.compute_gpu = 1;
    nt.stream_id = 0;
    nt._t = 1.025;
    NrnThreadMembList tml{};
    tml.index = 9;
    tml.ml = ml;
    nt.tml = &tml;

    REQUIRE(net_receive_buffer_enqueue(&nt, ml, 5, 2, 3.0));
    upload_net_receive_buffer_to_device(ml);
    update_net_receive_buffer(&nt);

    REQUIRE(detail::net_receive_buffer_device_cnt(ml->_net_receive_buffer) == 1);

    free_net_receive_buffer(ml);
#endif
}