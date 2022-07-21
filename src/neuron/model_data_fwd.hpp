#pragma once
namespace neuron {
struct Model;
inline Model& model();
namespace container {
template <typename>
struct generic_handle;
namespace utils {
template <typename T>
generic_handle<T> find_generic_handle(T*);
}
}  // namespace container
}  // namespace neuron