#pragma once
#include "neuron/container/mechanism_identifier.hpp"
#include "neuron/container/soa_container.hpp"

namespace neuron::container::Mechanism {
namespace field {
/** @brief Catch-all for floating point per-instance variables in the MOD file.
 *
 *  @todo Update the code generation so we get some hh_data = soa<hh_identifier,
 *  hh_a, hh_b, ...> type instead of fudging things this way.
 */
struct PerInstanceFloatingPointField {
    constexpr PerInstanceFloatingPointField(std::size_t num_copies)
        : m_num_copies{num_copies} {}
    /** @brief How many copes of this column should be created?
     */
    constexpr std::size_t num_instances() const {
        return m_num_copies;
    }
    using type = double;

  private:
    std::size_t m_num_copies{};
};
}  // namespace field

/** @brief Underlying storage for all instances of a particular Mechanism.
 */
struct storage: soa<storage, identifier, field::PerInstanceFloatingPointField> {
    using base_type = soa<storage, struct identifier, field::PerInstanceFloatingPointField>;
    storage(short mech_type, std::string name, std::size_t num_floating_point_fields)
        : base_type{field::PerInstanceFloatingPointField{num_floating_point_fields}}
        , m_mech_name{std::move(name)}
        , m_mech_type{mech_type} {
        std::cout << "mechanism " << m_mech_name << " has " << num_floating_point_fields
                  << " floating point fields and type " << m_mech_type << '\n';
    }
    [[nodiscard]] auto name() const {
        return m_mech_name;
    }
    [[nodiscard]] constexpr short type() const {
        return m_mech_type;
    }
    [[nodiscard]] constexpr std::size_t num_floating_point_fields() const {
        return get_tag<field::PerInstanceFloatingPointField>().num_instances();
    }

  private:
    std::string m_mech_name{};
    short m_mech_type{};
};

/** @brief Owning identifier for a row in the Mechanism storage;
 */
using owning_identifier = owning_identifier_base<storage, identifier>;
}  // namespace neuron::container::Mechanism
