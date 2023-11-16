#include <vector>
#include <random>
#include <iostream>
#include <algorithm> // for copy
#include <iterator> // for ostream_iterator
#include <unordered_set>
#include <chrono>
#include <functional>

// #include <format>

using std::chrono::high_resolution_clock;
using std::chrono::nanoseconds;
using std::chrono::milliseconds;

template<typename T>
void debug_print(const std::vector<T>& v, const std::vector<int>& p) {
    std::cout << "v" << std::endl;
    std::copy(v.begin(), v.end(), std::ostream_iterator<int>(std::cout, " "));
    std::cout << std::endl;
    std::cout << "p" << std::endl;
    std::copy(p.begin(), p.end(), std::ostream_iterator<int>(std::cout, " "));
    std::cout << std::endl;
}

// From https://devblogs.microsoft.com/oldnewthing/20170102-00/?p=95095
// Issue is that indices are copied by value
template<typename T>
void
apply_permutation1(
    std::vector<T>& v,
    std::vector<int> indices)
{
    for (size_t i = 0; i < indices.size(); i++) {
        T t{std::move(v[i])};
        auto current = i;
        while (i != indices[current]) {
            auto next = indices[current];
            v[current] = std::move(v[next]);
            indices[current] = current;
            current = next;
        }
        v[current] = std::move(t);
        indices[current] = current;
    }
}

// From https://devblogs.microsoft.com/oldnewthing/20170102-00/?p=95095
// A different version of apply_permutation1
// Issue is that indices are copied by value
template<typename T>
void
apply_permutation2(
    std::vector<T>& v,
    std::vector<int> indices)
{
    using std::swap; // to permit Koenig lookup
    for (size_t i = 0; i < indices.size(); i++) {
        auto current = i;
        while (i != indices[current]) {
            auto next = indices[current];
            swap(v[current], v[next]);
            indices[current] = current;
            current = next;
        }
        indices[current] = current;
    }
}

// I forgot where I found that but it's really slow (O(N^2))
template<typename T>
void
apply_permutation3(
    std::vector<T>& A,
    std::vector<int>& p)
{
    using std::swap; // to permit Koenig lookup
    size_t ind{};
    double temp{};
    for (size_t i = 0; i < p.size(); i++) {
        ind = p[i];
        while (ind < i) {
            ind = p[ind];
        }
        swap(A[i], A[ind]);
    }
}

// From https://cstheory.stackexchange.com/questions/6711/complexity-of-applying-a-permutation-in-place
// void compute_permutation(Object[] array, int[] pi)
// {
//     for(int i=0; i<array.length; i++)
//     {
//         if (pi[i] >= 0)
//         {
//             for(int j=i, k; i != (k = pi[j]); j = k)
//             {
//                 Object tmp = array[j];
//                 array[j] = array[k];
//                 array[k] = tmp;
//                 pi[j] = ~pi[j];
//             }
//         }
//     }

//     for(int i=0; i<pi.length; i++) pi[i] = ~pi[i];
// }

// TODO: C++ version not working
// template<typename T>
// void
// apply_permutation4(
//     std::vector<T>& v,
//     std::vector<int>& indices)
// {
//     debug_print(v,indices);
//     using std::swap; // to permit Koenig lookup
//     for (size_t i = 0; i < indices.size(); i++) {
//         if (indices[i] >= 0) {
//             auto k = indices[i];
//             for (size_t j = i; j != indices[j]; j = k) {
//                 swap(v[j], v[k]);
//                 k = indices[j];
//                 indices[j] = -indices[j];
//                 debug_print(v,indices);
//             }
//         }
//     }
//     for (auto& indice : indices) {
//         indice = -indice;
//     }
// }

// Simpler and better way for the current algorithm
template<typename T>
void
apply_permutation5(
    std::vector<T>& v,
    std::vector<int>& indices)
{
    std::vector<T> new_v;
    new_v.reserve(v.size());
    std::transform(indices.begin(), indices.end(), std::back_inserter(new_v), [&v](int ind) { return v[ind]; });
    v = std::move(new_v);
}

// Optimized algorithm based on https://devblogs.microsoft.com/oldnewthing/20170102-00/?p=95095
// and `Option 5` from https://cstheory.stackexchange.com/a/52365
// O(N) runtime and no O(1) space
// `indices` need to be threadsafe and writable (they return to their original state at the end)
template<typename T>
void
apply_permutation6(
    std::vector<T>& v,
    std::vector<int>& indices)
{
    using std::swap; // to permit Koenig lookup
    for (size_t i = 0; i < indices.size(); i++) {
        if (indices[i] >= 0) {
            auto current = i;
            while (indices[current] != i) {
                const auto next = indices[current];
                swap(v[current], v[next]);
                // -1 is needed because otherwise there is an issue if
                // indices[current] is 0
                indices[current] = -indices[current]-1;
                current = next;
            }
            indices[current] = -indices[current]-1;
        }
    }
    std::transform(indices.begin(), indices.end(), indices.begin(), [](int ind) { return -ind-1; });
}

// Similar to apply_permutation6 but with extra boolean vector
// to find visited elements
// Runtime complexity O(N) but space complexity O(N)
template<typename T>
void
apply_permutation7(
    std::vector<T>& v,
    const std::vector<int>& indices)
{
    std::vector<bool> visited(indices.size(), false);
    using std::swap; // to permit Koenig lookup
    for (size_t i = 0; i < indices.size(); i++) {
        if (!visited[i]) {
            auto current = i;
            while (indices[current] != i) {
                const auto next = indices[current];
                swap(v[current], v[next]);
                visited[current] = true;
                current = next;
            }
            visited[current] = true;
        }
    }
}

template<typename T>
void apply_permutation8(T*& v, std::vector<int>& indices, int n) {
    auto new_v = new T[n];
    for (auto i = 0; i < indices.size(); ++i) {
        new_v[i] = v[indices[i]];
    }
    std::swap(v, new_v);
    delete[] new_v;
}

template<typename T, typename F, int n>
void run_experiment(std::vector<T>& v, std::vector<int>& p, F f) {
    const auto num_experiments = 100;

    size_t sum_times{};
    for (auto experiment = 0; experiment < num_experiments; ++experiment) {
        for (int i = 0; i < v.size(); ++i) {
            v[i] = i;
        }

        auto start = high_resolution_clock::now();
        f(v, p);
        auto end = high_resolution_clock::now();
        sum_times += duration_cast<nanoseconds>(end - start).count();
    }
    std::cout << "  apply_permutation" << n << ": "
                << sum_times / num_experiments << " ns"
                << std::endl;
    if (v != p) {
        std::cout << "v and p after apply_permutation" << n << " are different" << std::endl;
        // debug_print(v,p);
    }
}

template<>
void run_experiment<int, decltype(apply_permutation8<int>), 8>(std::vector<int>& v, std::vector<int>& p, decltype(apply_permutation8<int>) f) {
    const auto num_experiments = 100;

    auto new_v = new int[v.size()];

    size_t sum_times{};
    for (auto experiment = 0; experiment < num_experiments; ++experiment) {
        for (int i = 0; i < v.size(); ++i) {
            new_v[i] = i;
        }

        auto start = high_resolution_clock::now();
        f(new_v, p, v.size());
        auto end = high_resolution_clock::now();
        sum_times += duration_cast<nanoseconds>(end - start).count();
    }
    std::cout << "  apply_permutation" << 8 << ": "
                << sum_times / num_experiments << " ns"
                << std::endl;
    for (int i = 0; i < v.size(); ++i) {
        if (new_v[i] != v[i]) {
            std::cout << "v and p after apply_permutation" << 8 << " are different" << std::endl;
        }
    }
    delete[] new_v;
}

int main(int argc, char *argv[]) {
    const auto N = argc > 1 ? std::atoi(argv[1]) : 1000000;
    std::cout << "Running experiment with " << N << " elements" << std::endl;
    std::vector<int> v;
    v.resize(N);
    std::vector<int> p;
    p.reserve(N);

    std::random_device dev;
    std::mt19937 rng(dev());
    std::uniform_int_distribution<std::mt19937::result_type> distN(0, N-1);

    std::unordered_set<int> p_unique;
    
    while (p_unique.size() < N) {
        auto rand_num = distN(rng);
        while (p_unique.contains(rand_num)) {
            rand_num = distN(rng);
        }
        p_unique.insert(rand_num);
    }

    for (int i = 0; i < N; ++i) {
        v[i] = i;
    }
    for (const auto& perm : p_unique) {
        p.push_back(perm);
    }

    // debug_print(v,p);

    // run_experiment<int, decltype(apply_permutation1<int>), 1>(v, p, apply_permutation1);

    // run_experiment<int, decltype(apply_permutation2<int>), 2>(v, p, apply_permutation2);

    // run_experiment<int, decltype(apply_permutation3<int>), 3>(v, p, apply_permutation3);
    
    // run_experiment<int, decltype(apply_permutation4<int>), 4>(v, p, apply_permutation4);

    run_experiment<int, decltype(apply_permutation5<int>), 5>(v, p, apply_permutation5);

    // run_experiment<int, decltype(apply_permutation6<int>), 6>(v, p, apply_permutation6);

    // run_experiment<int, decltype(apply_permutation7<int>), 7>(v, p, apply_permutation7);

    run_experiment<int, decltype(apply_permutation8<int>), 8>(v, p, apply_permutation8);

}
