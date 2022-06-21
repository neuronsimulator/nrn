#include <RNG_random123.h>
#include <Weibull_random123.h>

double Weibull_random123::operator()()
{
    return(d(*(generator()->get_random_generator())));
}
