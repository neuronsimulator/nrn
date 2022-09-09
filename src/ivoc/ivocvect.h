#pragma once
#include "nrnmutdec.h"

#include <vector>
#include <numeric>
#include <algorithm>

using ParentVect = std::vector<double>;
struct Object;

class IvocVect {
  public:
    IvocVect(Object* obj = nullptr);
    IvocVect(int, Object* obj = nullptr);
    IvocVect(int, double, Object* obj = nullptr);
    IvocVect(IvocVect&, Object* obj = nullptr);
    ~IvocVect();

    Object** temp_objvar();
    int buffer_size();
    void buffer_size(int);
    void label(const char*);

    double& elem(int n) {
        return vec_.at(n);
    }

    std::vector<double>& vec() {
        return vec_;
    }

    double* data() {
        return vec_.data();
    }

    size_t size() const {
        return vec_.size();
    }

    void resize(size_t n);
    void resize(size_t n, double fill_value);

    double& operator[](size_t index) {
        return vec_.at(index);
    }

    auto begin() -> std::vector<double>::iterator {
        return vec_.begin();
    }

    auto end() -> std::vector<double>::iterator {
        return vec_.end();
    }

    void push_back(double v) {
        vec_.push_back(v);
    }


    void mutconstruct([[maybe_unused]] int mkmut) {
#if NRN_ENABLE_THREADS
        if (!mut_)
            MUTCONSTRUCT(mkmut)
#endif
    }
    void lock() {
        MUTLOCK
    }
    void unlock() {
        MUTUNLOCK
    }

  public:
    // intended as friend static Object** temp_objvar(IvocVect*);
    Object* obj_;  // so far only needed by record and play; not reffed
    char* label_;
    std::vector<double> vec_;  // std::vector holding data
    MUTDEC
};

template <class InputIterator>
double var(InputIterator begin, InputIterator end) {
    const size_t size = end - begin;
    const double sum = std::accumulate(begin, end, 0.0);
    const double m = sum / size;

    double accum = 0.0;
    std::for_each(begin, end, [&](const double d) { accum += (d - m) * (d - m); });

    return accum / (size - 1);
}

template <class InputIterator>
double stdDev(InputIterator begin, InputIterator end) {
    return sqrt(var(begin, end));
}

void vector_delete(IvocVect*);
Object** vector_temp_objvar(IvocVect*);

int is_vector_arg(int);
char* vector_get_label(IvocVect*);
void vector_set_label(IvocVect*, char*);

// olupton 2022-01-21: backwards compatibility
using Vect = IvocVect;
