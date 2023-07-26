#pragma once
#include "membdef.h"
#include "neuron/container/node.hpp"
#include "neuron/container/soa_container.hpp"

namespace neuron::container::Node {
/** @brief Underlying storage for all Nodes.
 */
struct storage: soa<storage,
                    field::AboveDiagonal,
                    field::Area,
                    field::BelowDiagonal,
                    field::Diagonal,
                    field::FastIMemSavD,
                    field::FastIMemSavRHS,
                    field::RHS,
                    field::Voltage> {
    [[nodiscard]] std::string_view name() const {
        return {};
    }
};

/**
 * @brief Non-owning handle to a Node.
 */
using handle = handle_interface<non_owning_identifier<storage>>;

/**
 * @brief Owning handle to a Node.
 */
struct owning_handle: handle_interface<owning_identifier<storage>> {
    using base_type = handle_interface<owning_identifier<storage>>;
    using base_type::base_type;
    /**
     * @brief Get a non-owning handle from an owning handle.
     */
    [[nodiscard]] handle non_owning_handle() {
        return non_owning_identifier<storage>{&underlying_storage(), id()};
    }
};
}  // namespace neuron::container::Node
