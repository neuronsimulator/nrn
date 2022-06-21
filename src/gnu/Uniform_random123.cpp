#include <RNG_random123.h>
#include <Uniform_random123.h>

double Uniform_random123::operator()()
{
    return(d(*(generator()->get_random_generator())));
};
