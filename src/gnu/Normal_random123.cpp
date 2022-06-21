#include <RNG_random123.h>
#include <Normal_random123.h>

double Normal_random123::operator()()
{
    return(d(*(generator()->get_random_generator())));
};
