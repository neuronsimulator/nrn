#pragma once

#include <RNG_random123.h>

class Random_random123 {
protected:
    RNG_random123 *pGenerator;
public:
    Random(RNG *generator);
    virtual ~Random() {}
    virtual double operator()() = 0;

    RNG *generator();
    void generator(RNG *p);
};


inline Random::Random(RNG *gen)
{
    pGenerator = gen;
}

inline RNG *Random::generator()
{
    return(pGenerator);
}

inline void Random::generator(RNG *p)
{
    pGenerator = p;
}

#endif
