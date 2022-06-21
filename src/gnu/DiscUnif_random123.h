#pragma once

#include <random>

#include <Random.h>
#include <RNG_random123.h>

class DiscreteUniform_random123: public Random_random123 {
protected:
    std::uniform_int_distribution<> d;

public:
    DiscreteUniform_random123(double low, double high, RNG_random123 *gen);

    double low();
    double low(double x);
    double high();
    double high(double x);

    virtual double operator()();
};


inline DiscreteUniform_random123::DiscreteUniform_random123(double low, double high, RNG_random123 *gen) : Random_random123(gen)
{
  d = std::uniform_int_distribution<>(low, high);
}

inline double DiscreteUniform_random123::low() {
  return d.min();
}

inline double DiscreteUniform_random123::low(double x) {
  double tmp = d.min();
  d.param(std::uniform_int_distribution<>::param_type(x, d.max()));
  return tmp;
}

inline double DiscreteUniform_random123::high() {
  return d.max();
}

inline double DiscreteUniform_random123::high(double x) {
  double tmp = d.max();
  d.param(std::uniform_int_distribution<>::param_type(d.min(), x));
  return tmp;
}
