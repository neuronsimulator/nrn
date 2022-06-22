#include <RNG_random123.h>
#include <NegExp_random123.h>

double NegativeExpntl_random123::operator()()
{
    return(d(*(generator()->get_random_generator())));
};
