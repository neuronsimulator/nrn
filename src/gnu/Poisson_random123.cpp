#include <RNG_random123.h>
#include <Poisson_random123.h>

double Poisson_random123::operator()()
{
    return(d(*(generator()->get_random_generator())));
};
