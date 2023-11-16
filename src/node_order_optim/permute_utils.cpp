#include <permute_utils.hpp>

template <typename T>
void forward_permute(std::vector<T>& data, const std::vector<int>& perm) {
    // Move data[perm[i]] to data[i]
    std::vector<T> new_data;
    new_data.reserve(data.size());
    std::transform(perm.begin(), perm.end(), std::back_inserter(new_data), [&data](int ind) { return data[ind]; });
    data = std::move(new_data);
}

template <typename T>
void forward_permute(T*& data, const std::vector<int>& perm, int size) {
    // Move data[perm[i]] to data[i]
    auto new_data = new T[size];
    for (auto i = 0; i < perm.size(); ++i) {
        new_data[i] = data[perm[i]];
    }
    std::swap(data, new_data);
    delete[] new_data;
}

int* inverse_permute(int* p, int n) {
    int* pinv = new int[n];
    for (int i = 0; i < n; ++i) {
        pinv[p[i]] = i;
    }
    return pinv;
}
