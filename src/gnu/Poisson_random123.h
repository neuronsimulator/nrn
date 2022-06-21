#pragma once

#include <random>

#include <Random.h>
#include <RNG_random123.h>

class Poisson_random123: public Random_random123 {
protected:
    std::poisson_distribution<> d;

public:
    Poisson_random123(double mean, RNG_random123 *gen);

    double mean();
    double mean(double x);

    virtual double operator()();
};


inline Poisson_random123::Poisson_random123(double mean, RNG_random123 *gen)
: Random_random123(gen) {
  d = std::poisson_distribution<>(mean);
}

inline double Poisson_random123::mean() {
  return d.mean();
}

inline double Poisson_random123::mean(double x) {
  double t = d.mean();
  d.param(std::poisson_distribution<>::param_type(x));
  return t;
}
