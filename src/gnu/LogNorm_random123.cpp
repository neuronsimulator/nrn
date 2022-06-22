#include <RNG_random123.h>
#include <LogNorm_random123.h>

double LogNormal_random123::operator()()
{
    return(std::exp(this->Normal_random123::operator()()));
}
