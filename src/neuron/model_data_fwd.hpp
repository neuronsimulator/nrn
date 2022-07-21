#pragma once
namespace neuron {
struct Model;
inline Model& model();
namespace container {
template <typename>
struct data_handle;
namespace utils {
template <typename T>
data_handle<T> find_data_handle(T*);
}
}  // namespace container
}  // namespace neuron