#include <sstream>
#include <stdexcept>

#include "random_construct.hpp"

std::vector<RandomVar> random_variables;

std::vector<std::string> get_ext_random_methods() {
    return {RANDOM_SETSEQ_METHOD, RANDOM_INIT_METHOD, RANDOM_SAMPLE_METHOD};
}

RandomVar::RandomVar(const std::string& n,
                     const std::string& dist,
                     const std::vector<std::string>& args)
    : name(n)
    , distribution(dist)
    , arguments(args) {
    // check if distribution is one of "NEGEXP", "NORMAL", or "UNIFORM"
    if (dist != "NEGEXP" && dist != "NORMAL" && dist != "UNIFORM") {
        throw std::invalid_argument("Invalid distribution for RANDOM variable " + n);
    }
    // check the number of arguments based on the distribution
    if ((dist == "NEGEXP" && args.size() != 1) || (dist == "NORMAL" && args.size() != 2) ||
        (dist == "UNIFORM" && args.size() != 2)) {
        throw std::invalid_argument("Invalid number of arguments for distribution " + dist);
    }
}

std::string RandomVar::get_random123_function_for_distribution() const {
    static const std::unordered_map<std::string, std::string> function_map = {
        {"NEGEXP", "nrnran123_negexp"},
        {"NORMAL", "nrnran123_normal"},
        {"UNIFORM", "nrnran123_uniform"}};
    auto it = function_map.find(distribution);
    if (it != function_map.end()) {
        return it->second;
    }
    throw std::runtime_error("Invalid distribution for Random123: " + distribution);
}

std::vector<RandomVar> get_random_variables() {
    return random_variables;
}

void add_random_variable(const RandomVar& r) {
    random_variables.emplace_back(r);
}

size_t get_num_random_variables() {
    return random_variables.size();
}

RandomVar get_random_variable(const std::string& name) {
    for (const auto& var: random_variables) {
        if (var.name == name) {
            return var;
        }
    }
    throw std::runtime_error("Can not find a RANDOM variable with name " + name);
}