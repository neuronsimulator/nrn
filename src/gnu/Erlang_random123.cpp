#include <RNG_random123.h>
#include <Erlang_random123.h>

double Erlang_random123::operator()()
{
    return(d(*(generator()->get_random_generator())));
};
