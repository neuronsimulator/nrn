#pragma once

#include <random>

#include <Random.h>
#include <RNG_random123.h>

class Binomial_random123 : public Random_random123 {
protected:
    std::binomial_distribution<> d;
public:
    Binomial_random123(int t, double p, RNG_random123 *generator);

    int t();
    int t(int xt);

    double p();
    double p(int xp);

    virtual double operator()();

};

inline Binomial_random123::Binomial_random123(int t, double p, RNG_random123 *gen) : Random_random123(gen) {
    d = std::binomial_distribution<>(t, p);
}

inline int Binomial_random123::t() {
    return d.t();
}

inline int Binomial_random123::t(int xt) {
    int tmp = d.t();
    d.param(std::binomial_distribution<>::param_type(xt, d.p()));
    return tmp;
}

inline double Binomial_random123::p() {
    return d.p();
}

inline double Binomial_random123::p(int xp) {
    double tmp = d.p();
    d.param(std::binomial_distribution<>::param_type(d.t(), xp));
    return tmp;
}
