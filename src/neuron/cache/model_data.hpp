#pragma once
#include <optional>
#include <vector>
namespace neuron::cache {
struct Thread {
    std::vector<std::size_t> mechanism_offset{};
};
struct Model {
    std::vector<Thread> thread{};
};
extern std::optional<Model> model;
}  // namespace neuron::cache