#include <math.h>

/*Some functions supported by numpy that aren't included in math.h
 * names and arguments match the wrappers used in rxdmath.py
 */

double factorial(const double x)
{
	return tgamma(x+1.);
}


double degrees(const double radians)
{
    return radians * (180. / M_PI);
}

void radians(const double degrees, double *radians)
{
    *radians = degrees * (M_PI / 180.);
}


double log1p(const double x)
{
	return log(x+1.);
}

double vtrap(const double x, const double y)
{
    if(fabs(x/y) < 1e-6)
    {
        return y*(1.0 - x/y/2.0);
    }
    else
    {
        return x/(exp(x/y) - 1.0);
    }
}
