#include <cstddef>
#include <vector>

template <typename T>
void forward_permute(std::vector<T>& data, const std::vector<int>& perm);

template <typename T>
void forward_permute(T*& data, const std::vector<int>& perm, int size);

template <typename T>
std::vector<T> inverse_permute(const std::vector<T> p);
