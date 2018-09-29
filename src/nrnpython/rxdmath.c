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
