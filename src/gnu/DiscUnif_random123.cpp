#include <RNG_random123.h>
#include <DiscUnif_random123.h>

double DiscreteUniform_random123::operator()()
{
    return(d(*(generator()->get_random_generator())));
};
