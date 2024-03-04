#include <algorithm>
#include <vector>

template <typename T>
void forward_permute(std::vector<T>& data, const std::vector<int>& perm) {
    // Move data[perm[i]] to data[i]
    std::vector<T> new_data;
    new_data.reserve(data.size());
    std::transform(perm.begin(), perm.end(), std::back_inserter(new_data), [&data](int ind) {
        return data[ind];
    });
    data = std::move(new_data);
}

template <typename T>
void forward_permute(T*& data, int data_size, const std::vector<int>& perm) {
    // Move data[perm[i]] to data[i]
    auto new_data = new T[data_size];
    for (auto i = 0; i < perm.size(); ++i) {
        new_data[i] = data[perm[i]];
    }
    // cannot std::swap because data may have been allocated by malloc
    // std::swap(data, new_data);
    for (auto i = 0; i < perm.size(); ++i) {
        data[i] = new_data[i];
    }
    delete[] new_data;
}

template <typename T>
std::vector<T> inverse_permute_vector(const std::vector<T>& p) {
    std::vector<T> pinv(p.size());
    for (int i = 0; i < p.size(); ++i) {
        pinv[p[i]] = i;
    }
    return pinv;
}
