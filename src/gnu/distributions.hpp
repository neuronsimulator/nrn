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

class Erlang: public Random {
public:
    Erlang(double mean, double variance, RNG *gen)
        : Random(gen)
        , d(mean * mean / variance, variance / mean)
    {}

    double operator()() override {
        return d(*generator());
    }
private:
    std::gamma_distribution<> d;
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

class Weibull: public Random {
public:
    Weibull(double alpha, double beta, RNG *gen)
        : Random(gen)
        , d(alpha, std::pow(beta, 1 / alpha))
    {}

    double operator()() override {
        return d(*generator());
    }
private:
    std::weibull_distribution<> d;
};
