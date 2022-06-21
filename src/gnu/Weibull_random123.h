#pragma once

#include <random>

#include <Random.h>
#include <RNG_random123.h>

class Weibull_random123: public Random_random123 {
protected:
    std::weibull_distribution<> d;
    
public:
    Weibull_random123(double alpha, double beta, RNG_random123 *gen);

    double alpha();
    double alpha(double x);

    double beta();
    double beta(double x);

    virtual double operator()();
};
    
inline Weibull_random123::Weibull_random123(double alpha, double beta, RNG_random123 *gen) : Random_random123(gen)
{
  d = std::weibull_distribution<>(alpha, beta);
}

inline double Weibull_random123::alpha() {
  return d.a();
}

inline double Weibull_random123::alpha(double x) {
  double tmp = d.a();
  d.param(std::weibull_distribution<>::param_type(x, d.b()));
  return tmp;
}

inline double Weibull_random123::beta() {
  return d.b();
}
inline double Weibull_random123::beta(double x) {
  double tmp = d.b();
  d.param(std::weibull_distribution<>::param_type(d.a(), x));
  return tmp;
};
