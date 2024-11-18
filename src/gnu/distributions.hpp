#pragma once
#include <random>
#include "Random.h"

class Binomial: public Random {
public:
    Binomial(int n, double u, RNG *gen)
        : Random(gen)
        , d(n, u)
    {}

    double operator()() override {
        return d(*generator());
    }
private:
    std::binomial_distribution<> d;
};
