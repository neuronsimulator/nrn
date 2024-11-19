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

class DiscreteUniform: public Random {
public:
    DiscreteUniform(long low, long high, RNG *gen)
        : Random(gen)
        , d(low, high)
    {}
    double operator()() override {
        return d(*generator());
    }

private:
    std::uniform_int_distribution<> d;
};
