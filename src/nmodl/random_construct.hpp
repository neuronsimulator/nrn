#pragma once

/**
 * @file
 * @brief Implementations to support RANDOM construct in NMODL
 */

#include <string>
#include <vector>
#include <unordered_map>

/**
 * @brief Represents a random variable with a specified distribution and arguments.
 */
struct RandomVar {
    std::string name;          ///< Name of the random variable.
    std::string distribution;  ///< Distribution type (e.g., "NEGEXP", "NORMAL", "UNIFORM").
    std::vector<std::string> arguments;  ///< Arguments associated with the distribution.

    /**
     * @brief Constructs a RandomVar object.
     * @param n Name of the random variable.
     * @param dist Distribution type.
     * @param args Arguments associated with the distribution.
     * @throws std::invalid_argument if the distribution or arguments are invalid.
     */
    RandomVar(const std::string& n, const std::string& dist, const std::vector<std::string>& args);

    /**
     * @brief Gets the corresponding Random123 function name for the distribution.
     * @return Random123 function name.
     * @throws std::runtime_error if the distribution is invalid.
     */
    std::string get_random123_function_for_distribution() const;
};

const static char* RANDOM_SAMPLE_METHOD = "sample";  ///< Method for random sampling.
const static char* RANDOM_INIT_METHOD = "init";      ///< Method for random initialization.
const static char* RANDOM_SETSEQ_METHOD = "setseq";  ///< Method for setting the random sequence.
const static char* RANDOM123_SETSEQ_METHOD = "nrnran123_setseq";  ///< Method for setting Random123
                                                                  ///< sequence.

/**
 * @brief Gets a vector of methods that can be used with RANDOM variables.
 * @return Vector of strings representing methods used with RANDOM variables.
 */
std::vector<std::string> get_ext_random_methods();

/**
 * @brief Adds new random variable.
 * @param r RandomVar object representing the RANDOM variable specified in NEURON block.
 */
void add_random_variable(const RandomVar& r);

/**
 * @brief Gets a vector of all random variables specified in NEURON block.
 * @return Vector of RandomVar objects representing all RANDOM variables.
 */
std::vector<RandomVar> get_random_variables();

/**
 * @brief Gets a specific RANDOM variable by name.
 * @param name Name of the RANDOM variable to retrieve.
 * @return RandomVar object representing the specified RANDOM variable.
 */
RandomVar get_random_variable(const std::string& name);

/**
 * @brief Gets the number of random variables declared in NEURON block.
 * @return Number of RANDOM variables.
 */
size_t get_num_random_variables();