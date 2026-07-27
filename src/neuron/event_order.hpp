#pragma once

/**
 * Opt-in total order for discrete-event batch flushes (testing / raster parity).
 *
 * Enable: NRN_DETERMINISTIC_EVENTS=1
 *
 * Primary: delivery time t (already enforced by the event queue).
 * Same-t ties: class, src_gid, tgt_gid, mech_type, pnt_instance, flag,
 * weight_index, sendtype, seq.
 *
 * SelfEvent is a first-class class rank (not a fake gid). Does not fix FP
 * association order for multi-PP atomic rhs/d accumulation.
 */

#include <cstdlib>
#include <tuple>

namespace neuron::event_order {

/** True when NRN_DETERMINISTIC_EVENTS=1 (process lifetime). */
[[nodiscard]] inline bool enabled() noexcept {
    static int const on = [] {
        char const* e = std::getenv("NRN_DETERMINISTIC_EVENTS");
        return e && e[0] == '1' && e[1] == '\0';
    }();
    return on != 0;
}

/**
 * Event class ranks for same-t total order (must match across NEURON CPU,
 * native GPU, CoreNEURON CPU/GPU). SelfEvent before NetCon matches artcell
 * drain-self-before-NetCon tendency.
 */
enum class Class : int {
    SelfEvent = 0,       // net_send / net_move self-events
    NetCon = 1,          // synaptic NetCon deliver
    PreSynThreshold = 2, // voltage threshold → PreSyn::send
    NetEvent = 3,        // net_event from POINT_PROCESS
    Other = 9
};

/** Map device NetSendBuffer sendtype {0=net_send,1=net_event,2=net_move}. */
[[nodiscard]] inline Class class_from_net_send_type(int sendtype) noexcept {
    switch (sendtype) {
    case 0:
        return Class::SelfEvent;
    case 1:
        return Class::NetEvent;
    case 2:
        return Class::SelfEvent;  // net_move
    default:
        return Class::Other;
    }
}

struct Key {
    double t{0};
    int class_rank{static_cast<int>(Class::Other)};
    int src_gid{-1};
    int tgt_gid{-1};
    int mech_type{-1};
    int pnt_instance{-1};
    double flag{0};
    int weight_index{-1};
    int sendtype{-1};
    int seq{0};  // last-resort: original buffer index
};

[[nodiscard]] inline bool less(Key const& a, Key const& b) noexcept {
    // t first (strict time order)
    if (a.t != b.t) {
        return a.t < b.t;
    }
    return std::tie(a.class_rank,
                    a.src_gid,
                    a.tgt_gid,
                    a.mech_type,
                    a.pnt_instance,
                    a.flag,
                    a.weight_index,
                    a.sendtype,
                    a.seq) < std::tie(b.class_rank,
                                      b.src_gid,
                                      b.tgt_gid,
                                      b.mech_type,
                                      b.pnt_instance,
                                      b.flag,
                                      b.weight_index,
                                      b.sendtype,
                                      b.seq);
}

/** Threshold hit batch: all same t; order by src_gid then slot/seq. */
[[nodiscard]] inline Key threshold_hit_key(int src_gid, int slot_or_seq) noexcept {
    Key k;
    k.t = 0;  // equal for the batch; queue uses nt->_t+teps on send
    k.class_rank = static_cast<int>(Class::PreSynThreshold);
    k.src_gid = src_gid;
    k.seq = slot_or_seq;
    return k;
}

/** NetSendBuffer entry (self / net_event / net_move). */
[[nodiscard]] inline Key net_send_key(double t,
                                      int sendtype,
                                      int tgt_gid,
                                      int mech_type,
                                      int pnt_instance,
                                      double flag,
                                      int weight_index,
                                      int seq) noexcept {
    Key k;
    k.t = t;
    k.class_rank = static_cast<int>(class_from_net_send_type(sendtype));
    k.src_gid = -1;
    k.tgt_gid = tgt_gid;
    k.mech_type = mech_type;
    k.pnt_instance = pnt_instance;
    k.flag = flag;
    k.weight_index = weight_index;
    k.sendtype = sendtype;
    k.seq = seq;
    return k;
}

}  // namespace neuron::event_order
