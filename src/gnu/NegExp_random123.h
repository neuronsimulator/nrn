#pragma once

#include <random>

#include <Random.h>
#include <RNG_random123.h>

class NegativeExpntl_random123: public Random_random123 {
protected:
    double pMean;
    double lambda;
    std::exponential_distribution<> d;
public:
    NegativeExpntl_random123(double xmean, RNG_random123 *gen);
    double mean();
    double mean(double x);

    virtual double operator()();
};


inline NegativeExpntl_random123::NegativeExpntl_random123(double xmean, RNG_random123 *gen)
: Random_random123(gen) {
  pMean = xmean;
  lambda = 1 / pMean;
  d = std::exponential_distribution<>(lambda);
}

inline double NegativeExpntl_random123::mean() {
  return pMean;
}
inline double NegativeExpntl_random123::mean(double x) {
  double t = pMean;
  pMean = x;
  lambda = 1 / pMean;
  d.param(std::exponential_distribution<>::param_type(lambda));
  return t;
}
