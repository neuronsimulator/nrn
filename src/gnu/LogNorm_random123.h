#pragma once

#include <random>

#include <Normal_random123.h>
#include <Random.h>
#include <RNG_random123.h>

class LogNormal_random123: public Normal_random123 {
protected:
    double logMean;
    double logVariance;
    void setState();
public:
    LogNormal_random123(double mean, double variance, RNG_random123 *gen);
    double mean();
    double mean(double x);
    double variance();
    double variance(double x);
    virtual double operator()();
};


inline void LogNormal_random123::setState()
{
    double m2 = logMean * logMean;
    pMean = log(m2 / sqrt(logVariance + m2) );
// from ch@heike.informatik.uni-dortmund.de:
// (was   pVariance = log((sqrt(logVariance + m2)/m2 )); )
    pStdDev = sqrt(log((logVariance + m2)/m2 )); 
}

inline LogNormal_random123::LogNormal_random123(double mean, double variance, RNG_random123 *gen)
    : Normal_random123(std::log(mean*mean / std::sqrt(variance + (mean*mean))), log((variance + (mean*mean))/(mean*mean) ), gen)
{
    logMean = mean;
    logVariance = variance;
    setState();
}

inline double LogNormal_random123::mean() {
    return logMean;
}

inline double LogNormal_random123::mean(double x)
{
    double t=logMean;
    logMean = x;
    setState();
    Normal_random123::mean(pMean);
    return t;
}

inline double LogNormal_random123::variance() {
    return logVariance;
}

inline double LogNormal_random123::variance(double x)
{
    double t=logVariance;
    logVariance = x;
    setState();
    Normal_random123::variance(std::sqrt(pStdDev));
    return t;
}
