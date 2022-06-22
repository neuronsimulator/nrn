#include <RNG_random123.h>
#include <Geom_random123.h>

double Geometric_random123::operator()()
{
    return(1. + d(*(generator()->get_random_generator())));
};
