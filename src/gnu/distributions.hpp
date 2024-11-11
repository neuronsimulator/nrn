#pragma once
#include <random>
#include "Random.h"

class Binomial: public Random {
public:
    Binomial(int n, double u, RNG *gen)
        : Random(gen)
        , d(n ,u)
    {}

    double operator()() {
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

    double operator()() {
        return d(*generator());
    }
private:
    std::uniform_int_distribution<> d;
};

class Geometric: public Random {
public:
    Geometric(double mean, RNG *gen)
        : Random(gen)
        , d(mean)
    {}

    double operator()() {
        return 1. + d(*generator());
    }
private:
    std::geometric_distribution<> d;
};

class LogNormal: public Random {
public:
    LogNormal(double mean, double variance, RNG *gen)
        : Random(gen)
        , d(std::log(mean * mean / std::sqrt(variance + (mean * mean))), std::sqrt(std::log(variance + (mean * mean)) / (mean * mean)))
    {}

    double operator()() {
        return d(*generator());
    }
private:
    std::lognormal_distribution<> d;
};

class NegativeExpntl: public Random {
public:
    NegativeExpntl(double mean, RNG *gen)
        : Random(gen)
        , d(mean)
    {}

    double operator()() {
        return d(*generator());
    }
private:
    std::exponential_distribution<> d;
};

class Normal: public Random {
public:
    Normal(double mean, double variance, RNG *gen)
        : Random(gen)
        , d(mean, std::sqrt(variance))
    {}

    double operator()() {
        return d(*generator());
    }

private:
    std::normal_distribution<> d;
};

class Poisson: public Random {
public:
    Poisson(double mean, RNG *gen)
        : Random(gen)
        , d(mean)
    {}

    double operator()() {
        return d(*generator());
    }

private:
    std::poisson_distribution<> d;

};

class Uniform: public Random {
public:
    Uniform(double low, double high, RNG *gen)
        : Random(gen)
        , d(low, high)
    {}

    double operator()() {
        return d(*generator());
    }

private:
    std::uniform_int_distribution<> d;
};

