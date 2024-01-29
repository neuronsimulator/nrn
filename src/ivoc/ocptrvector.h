#pragma once
#include "neuron/container/data_handle.hpp"
#include "objcmd.h"

#include <cstddef>

struct OcPtrVector {
    OcPtrVector(std::size_t sz);
    virtual ~OcPtrVector();
    [[nodiscard]] std::size_t size() const {
        return pd_.size();
    }
    void resize(int);
    void pset(int i, neuron::container::data_handle<double> dh);
    [[nodiscard]] double getval(int);
    void setval(int, double);
    void scatter(double*, int sz);
    void gather(double*, int sz);

  public:
    std::vector<neuron::container::data_handle<double>> pd_{};
    char* label_{};
};
