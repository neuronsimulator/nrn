#pragma once

#include <random>

#include <Random.h>
#include <RNG_random123.h>

class Normal_random123: public Random_random123 {
protected:
    std::normal_distribution<> d;

public:
    Normal_random123(double mean, double stddev, RNG_random123 *gen);
    double mean();
    double mean(double x);
    double variance();
    double variance(double x);
    virtual double operator()();
};


inline Normal_random123::Normal_random123(double mean, double stddev, RNG_random123 *gen)
: Random_random123(gen) {
  d = std::normal_distribution<>(mean, stddev);
}

inline double Normal_random123::mean() {
  return d.mean();
}

inline double Normal_random123::mean(double x) {
  double t = d.mean();
  d.param(std::normal_distribution<>::param_type(x, d.stddev()));
  return t;
}

inline double Normal_random123::variance() {
  return d.stddev() * d.stddev();
}

inline double Normal_random123::variance(double x) {
  auto new_stddev = std::sqrt(x);
  double t = d.stddev() * d.stddev();
  d.param(std::normal_distribution<>::param_type(d.mean(), new_stddev));
  return t;
};
