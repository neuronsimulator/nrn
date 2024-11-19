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

class Poisson: public Random {
public:
    Poisson(double mean, RNG *gen)
        : Random(gen)
        , d(mean)
    {}

    double operator()() override {
        return d(*generator());
    }
private:
    std::poisson_distribution<> d;
};
