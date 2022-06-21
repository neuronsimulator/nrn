#include <RNG_random123.h>
#include <Binomial_random123.h>

double Binomial_random123::operator()()
{
    return(d(*(generator()->get_random_generator())));
};
