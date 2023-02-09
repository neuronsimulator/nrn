#pragma once
#include "neuron/container/non_owning_soa_identifier.hpp"
namespace neuron::scopmath {
template <typename MechRange>
struct row_view {
    row_view(MechRange* ml, std::size_t iml)
        : m_iml{iml}
        , m_ml{ml} {}
    [[nodiscard]] double& operator[](container::field_index ind) {
        return m_ml->data(m_iml, ind);
    }
    [[nodiscard]] double const& operator[](container::field_index ind) const {
        return m_ml->data(m_iml, ind);
    }
    [[nodiscard]] double& operator[](std::size_t col) {
        return m_ml->data(m_iml, col);
    }
    [[nodiscard]] double const& operator[](std::size_t col) const {
        return m_ml->data(m_iml, col);
    }

  private:
    std::size_t m_iml{};
    MechRange* m_ml{};
};
}  // namespace neuron::scopmath
