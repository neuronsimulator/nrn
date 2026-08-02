#pragma once
/**
 * @file point_process_access.hpp
 * @brief Internal helper to get a PointProcess SoA handle from Point_process*.
 *
 * Not for MOD files / section_fwd.hpp — only NEURON library TUs that already
 * link model_data.
 */
#include "neuron/container/network/point_process.hpp"
#include "neuron/model_data.hpp"
#include "section_fwd.hpp"

#include <cassert>

namespace neuron::container::network {

/** @brief Non-owning handle to the dual-write SoA row for @p pnt. */
inline PointProcess::handle point_process_soa(Point_process* pnt) {
    assert(pnt);
    assert(pnt->_soa_id);
    return PointProcess::handle{
        non_owning_identifier<PointProcess::storage>{&neuron::model().point_processes(),
                                                     pnt->_soa_id}};
}

inline PointProcess::handle point_process_soa(Point_process const* pnt) {
    return point_process_soa(const_cast<Point_process*>(pnt));
}

}  // namespace neuron::container::network
