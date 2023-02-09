#pragma once
#include "neuron/container/mechanism.hpp"
#include "neuron/container/soa_container.hpp"

#include <string_view>

namespace neuron::container::Mechanism {
/**
 * @brief Underlying storage for all instances of a particular Mechanism.
 *
 * To mitigate Python wheel ABI issues, a basic set of methods are defined in .cpp code that is
 * compiled as part of NEURON.
 */
struct storage: soa<storage, field::FloatingPoint> {
    using base_type = soa<storage, field::FloatingPoint>;
    // Defined in .cpp to avoid instantiating base_type constructors too often.
    storage(short mech_type, std::string name, std::vector<Variable> floating_point_fields = {});
    /**
     * @brief Access floating point values.
     */
    [[nodiscard]] double& fpfield(std::size_t instance, int field, int array_index = 0);
    /**
     * @brief Access floating point values.
     */
    [[nodiscard]] double const& fpfield(std::size_t instance, int field, int array_index = 0) const;
    /**
     * @brief Access floating point values.
     */
    [[nodiscard]] data_handle<double> fpfield_handle(non_owning_identifier_without_container id,
                                                     int field,
                                                     int array_index = 0);
    /**
     * @brief The name of this mechanism.
     */
    [[nodiscard]] std::string_view name() const;
    /**
     * @brief The type of this mechanism.
     */
    [[nodiscard]] short type() const;
    /**
     * @brief Pretty printing for mechanism data structures.
     */
    friend std::ostream& operator<<(std::ostream& os, storage const& data);

  private:
    short m_mech_type{};
    std::string m_mech_name{};
};

/**
 * @brief Non-owning handle to a Mechanism instance.
 */
using handle = handle_interface<non_owning_identifier<storage>>;

/**
 * @brief Owning handle to a Mechanism instance.
 */
using owning_handle = handle_interface<owning_identifier<storage>>;
}  // namespace neuron::container::Mechanism
